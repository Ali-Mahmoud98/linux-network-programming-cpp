<div dir="rtl" lang="ar">

# الدرس الأول: ليه قراءتين من TCP مش شرط يكونوا رسالتين؟

تخيّل إنك بتكتب سيرفر أوامر. العميل بعت الأمر <code dir="ltr">PING</code> وبعده <code dir="ltr">PONG</code> على نفس الاتصال. أول قراءة عند السيرفر رجّعت <code dir="ltr">PI</code> بس. **هل الاتصال ضيّع بايتات، ولا السيرفر استعجل لما اعتبر إن ده أمر كامل؟**

## إيه اللي TCP بيحافظ عليه؟

نفترض إن العميل بعت الأربع بايتات بتوع كل أمر بنجاح، من غير مسافة ولا سطر جديد بينهم. بالنسبة للتطبيق دول **أمرين**، لكن بالنسبة لـ TCP دول **تيار واحد من ٨ بايتات بالترتيب ده:** <code dir="ltr">PINGPONG</code>. مفيش علامة على السلك بتقول «هنا خلص أول استدعاء إرسال».

عشان كده، كل صف في الجدول ده احتمال صحيح **لنفس الإرسال**:

<table dir="rtl">
  <thead><tr><th>القراءة الأولى</th><th>القراءة التانية</th><th>اللي حصل</th></tr></thead>
  <tbody>
    <tr><td><code dir="ltr">PING</code></td><td><code dir="ltr">PONG</code></td><td>حدود القراءات وافقت حدود الأوامر بالصدفة.</td></tr>
    <tr><td><code dir="ltr">PINGPONG</code></td><td>مش محتاجين قراءة تانية للبايتات دي</td><td>قراءة واحدة شالت أمرين.</td></tr>
    <tr><td><code dir="ltr">PI</code></td><td><code dir="ltr">NGPONG</code></td><td>القراءة التانية فيها آخر الأمر الأول وكل الأمر التاني.</td></tr>
  </tbody>
</table>

الدفعة اللي بترجع من قراءة واحدة بنسميها <span dir="ltr">chunk</span>. حجمها بيعتمد على البايتات المتاحة وقت القراءة ومساحة الـ buffer اللي اديته للسيرفر. **حجم الـ buffer حد أقصى للقراءة، مش طول الرسالة.** والـ chunk مش لازم يساوي TCP segment واحد من اللي ممكن تشوفهم في التقاط الشبكة.

## طب ليه الـ echo شغّال؟

نتتبّع الصف الأخير في كود اللاب:

1. [جلسة TCP](../Practical-Implementation/src/transport/tcp/TcpSession.cpp) بتقرا بـ <code dir="ltr">async_read_some()</code>، وبتدي <code dir="ltr">on_bytes()</code> البايتات اللي وصلت فعلًا. في المرة الأولى هما <code dir="ltr">PI</code>.
2. [البروتوكول الخام](../Practical-Implementation/include/netserver/protocols/raw/RawStreamProtocol.hpp) بيحوّل الدفعة دي لـ <code dir="ltr">Message</code> واحدة من غير ما يستنى باقي الأمر. لما توصله الدفعة التانية، بيطلّع رسالة تانية فيها <code dir="ltr">NGPONG</code>.
3. [تطبيق الـ echo](../Practical-Implementation/include/netserver/app/EchoHandler.hpp) بيرد بمحتوى كل رسالة زي ما هو. بعد رسائل الترحيب، لو العميل جمع بايتات الرد هيلاقي <code dir="ltr">PINGPONG</code> تاني، حتى لو قرأها بتقسيم مختلف.

**نجاح الـ echo بيثبت إن البايتات رجعت بنفس الترتيب، مش إن السيرفر عرف حدود الأوامر.** لو بدّلنا الـ echo بتطبيق بينفّذ أمر مع كل رسالة، هيحاول ينفّذ <code dir="ltr">PI</code> كأمر، رغم إن العميل عمره ما بعت أمر بالاسم ده.

## جرّب تقسيم القراءة بنفسك

بدل ما تستنى الشبكة تقسّم البايتات بشكل معيّن، ادّي البروتوكول نفس الدفعتين مباشرة. كل سطر مطبوع هنا بيمثّل **رسالة شافها التطبيق**؛ السطر الجديد من الطباعة، مش من البايتات اللي بعتها العميل.

<div dir="ltr">

```cpp
#include <iostream>
#include <string_view>
#include "netserver/protocols/raw/RawStreamProtocol.hpp"

int main() {
    netserver::RawStreamProtocol raw;
    for (std::string_view chunk : {"PI", "NGPONG"}) {
        const auto events = raw.on_bytes(chunk.data(), chunk.size());
        for (const auto& message : events.messages) {
            std::cout << message.payload << '\n';
        }
    }
}
```

</div>

الناتج سطرين: <code dir="ltr">PI</code> وبعده <code dir="ltr">NGPONG</code>. جرّب تغيّر الدفعتين إلى دفعة واحدة <code dir="ltr">{"PINGPONG"}</code>: نفس البايتات، لكن التطبيق هيشوف رسالة واحدة بس. دي تجربة لسلوك <code dir="ltr">RawStreamProtocol</code>، مش تنبؤ بتوقيت الشبكة.

**سؤال قبل الدرس التاني:** لو كبّرنا الـ buffer، نقدر نضمن إن كل قراءة فيها أمر واحد؟ لأ. مساحة أكبر ممكن تستوعب أكتر من أمر، وقراءة قصيرة لسه ممكن ترجع قبل اكتمال أمر واحد. اللي ناقص مش مساحة؛ اللي ناقص **قاعدة متفق عليها بين العميل والسيرفر تحدد نهاية الرسالة**. دي فكرة الـ <span dir="ltr">framing</span> اللي هنجرّبها في [الدرس التاني: إزاي نحدد حدود الرسالة فوق TCP؟](2_tcp-framing-delimiters-and-length-prefix.md).

</div>