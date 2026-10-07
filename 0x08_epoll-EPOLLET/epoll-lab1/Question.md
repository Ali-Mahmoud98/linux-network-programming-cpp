# ليه بنستخدم `accept()` داخل loop

لأن مع `EPOLLET`، **ظهور `EPOLLIN` على الـ listening socket لا يعني أن هناك connection واحدة فقط**. معناه أن الـ socket أصبح لديه connection(s) جاهزة للـ `accept()`.

والقاعدة في Edge-Triggered هي:

> **لما يجيلك event، استهلك كل الـ work المتاح لحد ما تحصل على `EAGAIN`.**

خلينا نفصلها.

---

## 1. الأول: ماذا يوجد داخل الـ listening socket؟

لما تعمل:

```cpp
listen(serverFd, SOMAXCONN);
```

الـ kernel يحتفظ بـ **queue للـ connections التي تم قبول TCP handshake لها ولكن التطبيق لم يستدعِ `accept()` عليها بعد**.

مثلًا:

```text
                 Kernel
        ┌───────────────────────┐
        │ Listening socket      │
        │                       │
        │ Connection Queue      │
        │                       │
        │ [Client A]            │
        │ [Client B]            │
        │ [Client C]            │
        └───────────┬───────────┘
                    │
                    │ accept()
                    ▼
               Your Server
```

---

# 2. ماذا يحدث عندما يصل Client؟

مثلًا Client A يتصل:

```text
Client A
   │
   │ TCP connection
   ▼
Kernel
   │
   ▼
Connection Queue

┌─────────┐
│ Client A│
└─────────┘
```

الـ listening socket أصبح قابلًا للـ `accept()`.

وبالتالي:

```text
epoll_wait()
     │
     ▼
EPOLLIN
     │
     ▼
accept()
```

---

# 3. طيب لماذا لا نعمل `accept()` مرة واحدة؟

افترض أن 3 clients وصلوا تقريبًا في نفس الوقت:

```text
Client A ──┐
Client B ──┼──→ Kernel
Client C ──┘
```

فالـ queue ممكن تكون:

```text
┌─────────┬─────────┬─────────┐
│ Client A│ Client B│ Client C│
└─────────┴─────────┴─────────┘
```

حصل:

```cpp
epoll_wait();
```

والـ kernel قال لك:

```text
serverFd → EPOLLIN
```

لو عملت:

```cpp
accept();
```

مرة واحدة فقط:

```text
accept() → Client A
```

تصبح:

```text
┌─────────┬─────────┐
│ Client B│ Client C│
└─────────┴─────────┘
```

لسه عندك connections جاهزة!

> ملاحظه: عند استخدام EPOLLIN بدون EPOLLET، نكون في Level-Triggered mode. إذا كانت هناك connections pending في الـ listening socket، يظل الـ socket في حالة readable، وبالتالي يمكن أن يعيد epoll_wait() حدث EPOLLIN طالما أن هناك connections لم يتم accept() عليها.
>
> أما مع EPOLLET، فعند وصول EPOLLIN يجب أن نستمر في استدعاء accept() داخل loop حتى ترجع EAGAIN/EWOULDBLOCK، وبذلك نضمن أننا استهلكنا كل الـ connections الجاهزة حاليًا.

---

# 4. هنا تظهر أهمية الـ loop

نقول:

```cpp
while (true)
{
    int clientFd = accept(serverFd, nullptr, nullptr);

    ...
}
```

فيحصل:

```text
accept() → Client A
accept() → Client B
accept() → Client C
accept() → EAGAIN
```

ثم:

```text
EAGAIN
   │
   ▼
No more connections currently
   │
   ▼
STOP
```

وده بالضبط معنى:

> **Drain the accept queue until EAGAIN.**

---

# 5. لماذا `EAGAIN` هنا بالذات؟

لأننا جعلنا `serverFd` non-blocking:

```cpp
setNonBlocking(serverFd);
```

فلو مفيش connections جاهزة:

```cpp
accept(serverFd, ...);
```

لن ينتظر.

بدل ذلك:

```text
accept()
   │
   └── EAGAIN
```

ومعناها هنا:

> "مفيش connection أخرى جاهزة حاليًا."

وده بالضبط اللي نحتاجه في الـ ET event loop.

---

# 6. تخيلها كأنها طابور

عندنا:

```text
                 ACCEPT QUEUE

        ┌───────┬───────┬───────┬───────┐
        │   A   │   B   │   C   │   D   │
        └───────┴───────┴───────┴───────┘
             ↑
           accept()
```

كل `accept()` يشيل واحد:

```text
accept() → A

        ┌───────┬───────┬───────┐
        │   B   │   C   │   D   │
        └───────┴───────┴───────┘
```

ثم:

```text
accept() → B
accept() → C
accept() → D
```

وفي النهاية:

```text
┌─────────────────┐
│      EMPTY      │
└─────────────────┘
```

فنستدعي:

```cpp
accept()
```

مرة أخرى، فيرجع:

```text
EAGAIN
```

فنقول:

```text
"خلصنا."
```

---

# 7. طب ليه الموضوع مهم أكثر مع EPOLLET؟

هنا النقطة الأهم.

مع **Level-Triggered**:

```text
Queue:
[A][B][C]

      ↓
   EPOLLIN
      ↓
accept() → A

Queue:
[B][C]

      ↓
still readable
      ↓
EPOLLIN again
```

يعني حتى لو أخذت Client واحد فقط، الـ socket ما زال readable، وبالتالي ممكن تحصل على event مرة أخرى.

لكن مع **Edge-Triggered**، أنت لا تريد الاعتماد على أن `epoll` سيكرر التنبيه لك بنفس الطريقة.

لذلك:

```text
EPOLLIN
   ↓
accept()
   ↓
accept()
   ↓
accept()
   ↓
EAGAIN
   ↓
DONE
```

---

# 8. ونفس الفكرة مع `recv()`

وده سبب إننا استخدمنا نفس pattern هنا:

```cpp
while (true)
{
    ssize_t n = recv(...);

    if (n > 0)
        continue;

    if (n == -1 &&
        (errno == EAGAIN || errno == EWOULDBLOCK))
        break;
}
```

يعني عندنا قاعدتين متطابقتين:

```text
Listening socket
       │
       ▼
   accept()
       │
       ├── Client A
       ├── Client B
       ├── Client C
       └── EAGAIN
```

و:

```text
Connected socket
       │
       ▼
     recv()
       │
       ├── data
       ├── data
       ├── data
       └── EAGAIN
```

---

## الخلاصة

في `EPOLLET`، لا تفكر:

> `EPOLLIN` → اعمل `accept()` مرة.

فكر:

> `EPOLLIN` → **هناك work متاح** → استهلكه كله → `EAGAIN` → توقف.

لذلك:

```cpp
while (true)
{
    accept();
}
```

ليست لأن `accept()` يحتاج loop بطبيعته، وإنما لأننا نريد **تفريغ الـ pending connection queue بالكامل** عندما يصلنا الـ edge.

والـ pattern الذي أريدك تحفظه هو:

```text
              EPOLLET
                 │
                 ▼
          "Work is available"
                 │
                 ▼
        Keep doing the operation
                 │
                 ▼
              EAGAIN
                 │
                 ▼
               STOP
```

وده ينطبق على:

```text
accept()  → EAGAIN → stop
recv()    → EAGAIN → stop
send()    → EAGAIN → stop
```

**وده هو قلب فهم `EPOLLET`.**
