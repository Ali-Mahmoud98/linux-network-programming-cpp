<div dir="rtl" lang="ar">

# الدرس التالت: مين ينقل البايتات، ومين يفهمها، ومين يرد؟

في [الدرس التاني](2_tcp-framing-delimiters-and-length-prefix.md) اتفقنا إن <code dir="ltr">00 00 00 04 50 49 4E 47</code> تمثّل رسالة <code dir="ltr">PING</code> **لو الطرفين اختاروا بروتوكول بادئة الطول**. طب لو بعتنا نفس البايتات لسيرفر اللاب الحالي، هل التطبيق هيشوف أربع بايتات ولا تمانية؟ الإجابة بتعتمد على البروتوكول وحدود القراءة، مش على نية المرسل. خلّينا نتتبّع مين بياخد كل قرار.

## رحلة رسالة واحدة في الكود الحالي

نفترض إن العميل قرأ رسائل الترحيب، وبعدين بعت <code dir="ltr">PING</code> من غير هيدر، ووصلت الأربع بايتات في قراءة واحدة:

<table dir="rtl">
  <thead><tr><th>المسؤول</th><th>السؤال اللي بيجاوب عليه</th><th>إيه اللي حصل في المثال؟</th></tr></thead>
  <tbody>
    <tr><td>النقل: <code dir="ltr">TcpSession</code></td><td>كام بايت وصلوا فعلًا؟</td><td>قرأت الجلسة ٤ بايتات من الـ socket بـ <code dir="ltr">async_read_some()</code>، ومرّرتهم لـ <code dir="ltr">on_bytes()</code>. الجلسة ما قررتش إن ده أمر كامل.</td></tr>
    <tr><td>البروتوكول: <code dir="ltr">RawStreamProtocol</code></td><td>البايتات دي تكوّن كام رسالة؟</td><td>القاعدة الحالية إن كل قراءة تبقى رسالة. رجّع رسالة محتواها <code dir="ltr">PING</code> في <code dir="ltr">ProtocolEvents::messages</code>، والجلسة سلّمتها للتطبيق.</td></tr>
    <tr><td>التطبيق: <code dir="ltr">EchoHandler</code></td><td>أعمل إيه بالرسالة؟</td><td>اختار يرجّع نفس الرسالة، فطلب <code dir="ltr">reply_channel-&gt;send(inbound.message)</code>. مش محتاج يعرف رقم الـ socket ولا نوع البروتوكول.</td></tr>
  </tbody>
</table>

**في الرجوع، القرار بيتقسم بنفس الطريقة.** التطبيق اختار *محتوى* الرد. [الجلسة](../Practical-Implementation/src/transport/tcp/TcpSession.cpp) بتنادي <code dir="ltr">encode()</code> عشان البروتوكول يحدد البايتات اللي هتتبعت. مع البروتوكول الخام الناتج هو <code dir="ltr">PING</code> زي ما هو؛ الجلسة بتحطه في طابور الكتابة وبتكتب على الـ socket. ولو عايزين الرد على أمر كامل يبقى <code dir="ltr">PONG</code> بدل <code dir="ltr">PING</code>، ده قرار [التطبيق](../Practical-Implementation/include/netserver/app/EchoHandler.hpp)، مش تغيير في طابور الكتابة أو شكل الهيدر.

## ليه الجلسة بتملك بروتوكول بدل ما تورث منه؟

الجلسة **مش نوع من البروتوكول**: شغلتها تفضل هي هي سواء الرسالة بتنتهي بسطر جديد أو بتبدأ بطول. عشان كده <code dir="ltr">TcpSession</code> بتاخد <code dir="ltr">std::unique_ptr&lt;IStreamProtocol&gt;</code> في الـ constructor. الكلاس اللي بيطبّق [واجهة البروتوكول](../Practical-Implementation/include/netserver/protocols/IStreamProtocol.hpp) فعلًا هو [RawStreamProtocol](../Practical-Implementation/include/netserver/protocols/raw/RawStreamProtocol.hpp). ده **تركيب**: نقدر نبدّل قاعدة فهم البايتات من غير ما نعمل كلاس جلسة جديد.

[نقطة تشغيل البرنامج](../Practical-Implementation/src/main.cpp) بتختار البروتوكول الخام عن طريق دالة إنشاء (<span dir="ltr">factory</span>). [السيرفر](../Practical-Implementation/src/transport/tcp/TcpServer.cpp) بيناديها لكل جلسة، بدل ما يشارك كائن بروتوكول واحد بين العملاء. ليه ده مهم رغم إن البروتوكول الخام مش بيخزّن بايتات؟ تخيّل بروتوكول بادئة الطول بتاع الدرس التاني:

<table dir="rtl">
  <thead><tr><th>العميل</th><th>البايتات الجديدة</th><th>المفروض يحصل إيه؟</th></tr></thead>
  <tbody>
    <tr><td>أ</td><td><code dir="ltr">00 00 00 04 50 49</code></td><td>لسه ناقص بايتين. بروتوكول «أ» يحتفظ بـ <code dir="ltr">PI</code> وما يسلّمش رسالة.</td></tr>
    <tr><td>ب</td><td><code dir="ltr">00 00 00 02 4F 4B</code></td><td>بروتوكول «ب» يبدأ من هيدر جديد ويسلّم <code dir="ltr">OK</code>. مالوش دعوة باللي «أ» مستنيه.</td></tr>
    <tr><td>أ</td><td><code dir="ltr">4E 47</code></td><td>دلوقتي بس تكتمل <code dir="ltr">PING</code> عند «أ».</td></tr>
  </tbody>
</table>

لو الاتنين شاركوا نفس حالة القراءة، أول بايتين من هيدر «ب» ممكن يتحسبوا تكملة لرسالة «أ». المشكلة دي ممكن تحصل حتى لو السيرفر شغّال على thread واحد: السبب خلط **اتصالين**، مش تشغيل دالتين في نفس اللحظة.

## تجربة من غير شبكة: هل البروتوكول شايف الهيدر؟

ده معنى <span dir="ltr">Sans-I/O</span> في اللاب: البروتوكول بياخد بايتات ويرجّع نتيجة؛ مش هو اللي بيعمل قراءة أو كتابة. عشان نجاوب على سؤال أول الدرس، نقدر نديه الإطار كله في استدعاء واحد، من غير سيرفر ولا عميل:

<div dir="ltr">

```cpp
#include <cassert>
#include <string>
#include "netserver/protocols/raw/RawStreamProtocol.hpp"

int main() {
    const char frame[] = {0, 0, 0, 4, 'P', 'I', 'N', 'G'};
    const std::string wire(frame, sizeof frame);
    netserver::RawStreamProtocol raw;

    const auto events = raw.on_bytes(wire.data(), wire.size());
    assert(events.messages.size() == 1);
    assert(events.messages.front().payload == wire);
    assert(raw.encode(events.messages.front()) == wire);
}
```

</div>

**النتيجة:** مع <code dir="ltr">RawStreamProtocol</code> وقراءة واحدة، التطبيق بيشوف رسالة فيها **الـ ٨ بايتات كلهم**، بما فيهم الأصفار. مع بروتوكول بادئة الطول المقترح، نفس الدخل المفروض يطلّع رسالة محتواها **٤ بايتات بس**: <code dir="ltr">PING</code>. ده اختبار تقدر تكتبه لما تضيف البروتوكول الجديد؛ مش سلوك منفّذ في اللاب دلوقتي.

**اختبر فهمك:** العميل بطيء وطابور الكتابة اتملى: تبص فين؟ في **النقل**، لأنه صاحب الطابور. طول الإطار المعلن أكبر من المسموح: تبص فين؟ في **البروتوكول**، لأنه صاحب قاعدة الطول، والجلسة هي اللي تنفّذ قرار الإغلاق. ولو الإطار سليم لكن الأمر نفسه مش معروف؟ دي مسؤولية **التطبيق**.

</div>