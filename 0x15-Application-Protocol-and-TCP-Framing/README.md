<div dir="rtl" lang="ar">

# لاب 0x15: بروتوكول التطبيق وحدود الرسائل فوق TCP

لو العميل بعت <code dir="ltr">PING</code> وبعدها <code dir="ltr">PONG</code>، السيرفر ممكن يقرا <code dir="ltr">PI</code> في مرة و<code dir="ltr">NGPONG</code> في المرة اللي بعدها. الـ echo هيرجّع نفس البايتات بالترتيب، لكن ده مش معناه إنه عرف فين انتهى الأمر الأول. **مين يحدد حدود الرسالة: كود الـ socket، ولا البروتوكول، ولا التطبيق؟**

اللاب ده بيكمّل [لاب 0x14](../0x14-Networking-basics-asio-tcp/README.md). هناك فصلنا الرد عن النقل. هنا فصلنا كمان **تفسير البايتات** عن نقلها، من غير ما نغيّر اللي بيشوفه عميل الـ echo.

## ابدأ بالدروس بالترتيب

1. [تيار البايتات وحدود الرسالة](Theory-and-Concepts/1_tcp-byte-stream-and-message-boundaries.md): ليه استدعاء إرسال واحد مش شرط يقابله قراءة واحدة؟ جرّب نفس البايتات بتقسيمين مختلفين.
2. [الفاصل ولا بادئة الطول؟](Theory-and-Concepts/2_tcp-framing-delimiters-and-length-prefix.md): قارن نهاية السطر بهيدر من ٤ بايتات، وتتبّع إمتى المستقبل يستنى وإمتى يسلّم رسالتين من قراءة واحدة.
3. [مين ينقل البايتات، ومين يفهمها، ومين يرد؟](Theory-and-Concepts/3_transport-protocol-and-application.md): تتبّع رحلة رسالة واحدة، وشوف ليه كل اتصال بياخد كائن بروتوكول مستقل وإزاي تختبره من غير شبكة.

## إيه اللي اتنفّذ؟

[جلسة TCP](Practical-Implementation/src/transport/tcp/TcpSession.cpp) بتدير الـ socket وطابور الكتابة. كل قراءة بتعدّي على <code dir="ltr">IStreamProtocol::on_bytes()</code>، وكل رد من التطبيق بيعدّي على <code dir="ltr">encode()</code> قبل ما يدخل الطابور. [تطبيق الـ echo](Practical-Implementation/include/netserver/app/EchoHandler.hpp) هو اللي بيختار رسائل الترحيب وبيقرّر يرجّع محتوى الرسالة زي ما هو.

[نقطة تشغيل السيرفر](Practical-Implementation/src/main.cpp) بتختار <code dir="ltr">RawStreamProtocol</code> لكل اتصال: كل دفعة بايتات من قراءة واحدة بتتبعت للتطبيق كرسالة، من غير إضافة هيدر أو انتظار فاصل. **يعني اللاب جهّز مكان الـ framing، لكن السيرفر الحالي مش بيطبّق فاصل نهاية السطر ولا بادئة الطول.**

لما تفتح الكود، [شرح التغييرات](Practical-Implementation/docs/0_code-changes-explaination.md) بيربط كل تعديل بالمشكلة اللي بيحلّها. و[رسومات UML](Practical-Implementation/docs/1_code-structure-and-UML.md) بتوضّح مين بيملك الجلسة والبروتوكول، وبتتتبّع الاتصال من فتحه لحد إغلاقه.

## ابنِ السيرفر وجرّبه

على Linux هتحتاج مترجم بيدعم <span dir="ltr">C++17</span>، و<span dir="ltr">CMake 3.22.1</span> أو أحدث، وهيدرات <span dir="ltr">standalone Asio</span> بحيث المترجم يلاقي <code dir="ltr">asio.hpp</code>. من جذر الريبو شغّل:

<div dir="ltr">

```bash
cd 0x15-Application-Protocol-and-TCP-Framing
cmake -S Practical-Implementation -B build -DDEBUG_TRACE=ON
cmake --build build
./build/asio-tcp-server
```

</div>

من ترمينال تانية اتصل بأداة <span dir="ltr">netcat</span>:

<div dir="ltr">

```bash
nc 127.0.0.1 8080
```

</div>

بعد رسالتي الترحيب، اكتب <code dir="ltr">PING</code> واضغط <span dir="ltr">Enter</span>. الرد هيكون <code dir="ltr">PING</code>، مش <code dir="ltr">PONG</code>، لأن التطبيق بيكرّر المحتوى ومش بيفسّره كأمر. نهاية السطر هنا جزء من البايتات، مش قاعدة بيستخدمها السيرفر لتحديد نهاية الرسالة. لإيقاف السيرفر اضغط <code dir="ltr">Ctrl+C</code> في الترمينال الأولى.

**الخطوة الجاية:** تنفيذ <code dir="ltr">LengthPrefixProtocol</code> واختباره ببايتات معروفة: رسالة مقسومة على قراءتين، ورسالتين في قراءة واحدة. معيار النجاح إن التطبيق يشوف نفس الرسائل، مهما اختلف تقسيم القراءات.

</div>
