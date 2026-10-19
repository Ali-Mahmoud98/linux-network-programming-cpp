# هل محتاج 2 context؟

**لأ، `io_context` واحد كفاية.** الـ `io_context` مش مربوط ببروتوكول معين. هو محرك الأحداث (event loop) اللي بيخدم أي عدد من الـ sockets والـ acceptors والـ timers، سواء TCP أو UDP أو غيرهم. فالسرفر بتاعك يتعمل كده:

### [Check Code](./main2.cpp)

Compile:
```bash
g++ main2.cpp -o test
```

# يعني إيه io_context بالظبط؟

## 1. الفكرة العامة

الـ `io_context` هو الوسيط بين برنامجك وبين نظام التشغيل في كل ما يخص الـ I/O غير المتزامن. جواه حاجتين أساسيتين:

- **Reactor/Proactor للـ OS:** بيستخدم الآلية الأصلية للنظام: `epoll` على Linux، `kqueue` على macOS/BSD، `IOCP` على Windows.
- **طابور handlers (completion queue):** لما عملية تخلص، الـ handler بتاعها (الـ lambda اللي إنت كتبتها) بيتحط في الطابور، وبيتنفذ جوه `run()`.

## 2. إيه اللي بيحصل في كودك خطوة بخطوة

1. `async_accept(...)`: بتقول للـ context "أنا عايز أعرف لما حد يتصل". الدالة **بترجع فورًا** من غير ما تستنى.
2. `io_context.run()`: هنا الـ thread الحالي بيدخل الـ event loop. بيستنى أحداث من الـ OS، ولما يحصل حدث بينفذ الـ handler المناسب.
3. لما client يتصل، الـ OS بيبلغ الـ context، والـ context بينفذ الـ lambda بتاعة الـ accept على نفس الـ thread اللي بينادي `run()`.
4. جوه الـ handler عملت `async_write`، فبقى في عملية جديدة معلقة، ودورة جديدة بنفس الطريقة.

النقطة المهمة: **الـ handlers بتتنفذ بس جوه thread بينادي `run()`**. مفيش حاجة بتتنفذ "في الخلفية" من نفسها.

## 3. متى يرجع run()؟

`run()` بيرجع لما **مفيش شغل معلق** (no outstanding work)، يعني مفيش async operation شغالة ولا handler مستني. وده بيفسر إن برنامجك الأصلي هيقفل بعد ما يبعت الرسالة لأول client. لو عايزه يفضل شغال لازم تعمل `async_accept` تاني من جوه الـ handler (accept loop)، أو تستخدم `asio::executor_work_guard`.

## 4. وظايف التشغيل

| الدالة | السلوك |
|---|---|
| `run()` | بيفضل يشتغل لحد ما الشغل يخلص أو `stop()` |
| `run_one()` | ينفذ handler واحد ويرجع (blocking) |
| `poll()` | ينفذ الـ handlers الجاهزة حالًا من غير ما يستنى |
| `poll_one()` | زي poll بس handler واحد |
| `stop()` | بيوقف الـ run() فورًا |
| `restart()` | لازم تناديها قبل run() تاني بعد stop() أو بعد ما يخلص الشغل |

الـ `poll()` مفيدة لو بتدمج Asio جوه game loop أو GUI loop عندك.

## 5. post و dispatch

الـ `io_context` مش للـ I/O بس، تقدر تبعتله شغل عادي:

```cpp
asio::post(io, []{ std::cout << "run on io thread\n"; });
```

ده بيحط الـ lambda في الطابور وتتنفذ في thread من اللي بينادوا `run()`. مفيد جدًا لنقل شغل من thread لـ thread بأمان.

## 6. الـ socket و علاقته بالـ context

الـ socket بياخد `io_context` في الـ constructor لأنه **I/O object مربوط بالـ context ده**. العمليات الـ async بتاعته بتتسجل في الـ context ده. تقدر تقول:

- الـ socket = "الطرف" اللي بيمثل الاتصال (file descriptor جواه).
- الـ io_context = "المشغّل" اللي بيراقب الـ descriptors دي ويشغل الـ handlers.

ملاحظتين مهمتين:

- الـ socket **مش copyable**، بس movable. عشان كده الـ capture بالـ reference أو `shared_ptr`.
- **ترتيب الـ destruction:** الـ socket لازم يتدمر قبل الـ io_context. في كودك ده تمام لأن الـ socket معرّف بعده (بيتدمر بالعكس).

## 7. الـ threads

- **thread واحد بينادي run():** كل الـ handlers متسلسلة، مفيش race conditions بين الـ handlers. ده الأبسط والأضمن وكفاية لأغلب السرفرات.
- **عدة threads بتنادي run() على نفس الـ context:** الـ handlers ممكن تتنفذ بالتوازي على threads مختلفة. هنا لازم تحمي الـ shared state، والأداة المعتادة هي الـ **strand**:

```cpp
auto strand = asio::make_strand(io);
// الـ handlers المربوطة بالـ strand دي مش هتتنفذ في نفس الوقت أبدًا
```

```cpp
std::vector<std::thread> pool;
for (int i = 0; i < 4; ++i)
    pool.emplace_back([&]{ io.run(); });
for (auto& t : pool) t.join();
```

## 8. إمتى تعمل أكتر من io_context؟

الأغلب مش محتاج، بس في حالات مبررة:

- **io_context per thread:** كل thread له context خاص وsockets خاصة بيه، فبتقلل الـ contention. ده نمط بتستخدمه سرفرات high-performance.
- **عزل أحمال:** مثلًا context للشبكة وcontext لشغل تاني ثقيل عشان ما يبطّئوش بعض.
- **مكتبات منفصلة:** لو كل مكتبة بتدير event loop خاص بيها.

لكن مجرد إن عندك TCP وUDP **مش سبب** لـ context تاني.

---

# الخلاصة

- `io_context` = event loop + طابور handlers، وبيخدم أي نوع sockets.
- `async_*` بتسجل وترجع فورًا، و`run()` هو اللي بينفذ الـ handlers.
- `run()` بيرجع لما الشغل المعلق يخلص.
- الـ socket مربوط بالـ context، ومش copyable.
- context واحد + thread واحد = أبسط تصميم، وبعد كده تكبّر بـ thread pool + strand لما تحتاج.
