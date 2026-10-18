خلينا نشرح الجزئين بالتفصيل، ونربطهم بالـ `Connection` class اللي بنبنيه باستخدام C++ و Asio، لأن الجزئين دول مهمين جدًا في فهم إدارة عمر الاتصال (Connection Lifetime)، وإلغاء العمليات الـ Asynchronous، وطريقة تخزين الـ Callbacks.

# الجزء الأول: شرح `shutdown()` و `close()` في Asio

الكود اللي عندنا:

```
asio::error_code ignored_ec;

// قفل الـ Socket بيلغي أي عملية async معلقة
m_socket.shutdown(
    asio::ip::tcp::socket::shutdown_both,
    ignored_ec
);

m_socket.close(ignored_ec);
```

الفكرة الأساسية إننا لما نقرر ننهي اتصال الـ Client، مش كفاية إننا نوقف التعامل معاه في الكود؛ لازم كمان ننهي الاتصال على مستوى الـ Socket ونتعامل مع أي عمليات Asynchronous لسه معلقة.

## 1. يعني إيه `asio::error_code ignored_ec`؟

خلينا نفك السطر ده:

```
asio::error_code ignored_ec;
```

الـ `error_code` هو كائن بيخزن معلومات عن الخطأ اللي حصل أثناء تنفيذ عملية معينة.

مثلًا، ممكن عملية `shutdown()` تفشل بسبب إن الاتصال اتقفل بالفعل، أو إن الـ Socket مش في حالة تسمح بتنفيذ العملية.

في Asio، تقدر تتعامل مع الأخطاء بطريقتين أساسيتين.

الطريقة الأولى: استخدام الـ Exceptions

```
m_socket.shutdown(
    asio::ip::tcp::socket::shutdown_both
);
```

هنا لو العملية فشلت، ممكن Asio ترمي `asio::system_error` exception.

الطريقة الثانية: استخدام `error_code`

```
asio::error_code ec;

m_socket.shutdown(
    asio::ip::tcp::socket::shutdown_both,
    ec
);

if (ec) {
    std::cout << "Error: " << ec.message() << '\n';
}
```

هنا بدل ما نعتمد على الـ exception، بنستقبل نتيجة العملية في `ec`، ونقدر نقرر نتصرف إزاي.

طيب، ليه سميناه `ignored_ec`؟

لأننا في الكود ده مش مهتمين بنتيجة العملية؛ إحنا بنحاول ننهي الاتصال، ولو العملية فشلت لأي سبب، هنحاول برضه ننفذ `close()`.

مهم: اسم `ignored_ec` مجرد اسم متغير اختاره المبرمج. مش كلمة محجوزة في C++، ومش معناها إن Asio بيتجاهل الأخطاء تلقائيًا.

## 2. يعني إيه `shutdown()`؟

بص على السطر ده:

```
m_socket.shutdown(
    asio::ip::tcp::socket::shutdown_both,
    ignored_ec
);
```

الـ `shutdown()` بتتحكم في اتجاهات الاتصال على مستوى الـ TCP Socket.

في TCP، عندك اتجاهين لنقل البيانات:

Server

اتجاه إرسال البيانات

Server → Client

اتجاه استقبال البيانات

Client → Server

Client

الـ TCP اتصال ثنائي الاتجاه (Full-Duplex)، يعني كل طرف ممكن يرسل ويستقبل في نفس الوقت.

الـ `shutdown()` بتسمح لك توقف الإرسال أو الاستقبال أو الاثنين معًا، من غير ما يكون هدفها الأساسي إغلاق الـ Socket نفسه.

### أنواع `shutdown`

| القيمة             | معناها                   |
| ------------------ | ------------------------ |
| `shutdown_receive` | إيقاف استقبال البيانات   |
| `shutdown_send`    | إيقاف إرسال البيانات     |
| `shutdown_both`    | إيقاف الإرسال والاستقبال |

في الكود بتاعنا:

```
asio::ip::tcp::socket::shutdown_both
```

إحنا بنطلب إيقاف الاتجاهين.

تقدر تتخيل إننا بنقول للـ Socket:

> أنا مش عايز أكمل إرسال أو استقبال بيانات على الاتصال ده.

لكن خلي بالك من نقطة مهمة: `shutdown()` مش هي نفسها `close()`. هي بتوقف اتجاهات الاتصال، بينما الـ Socket object لسه ممكن يكون مفتوحًا من ناحية موارد النظام.

كمان `shutdown_both` مش معناها إن كل البيانات الموجودة في الـ buffers اختفت فورًا، ولا إنها تضمن إن الطرف الآخر استقبل كل البيانات السابقة.

## 3. يعني إيه `close()`؟

بعد `shutdown()`، عندنا:

```
m_socket.close(ignored_ec);
```

الـ `close()` بتقفل الـ Socket وتحرر مورد الـ socket الخاص بنظام التشغيل المرتبط بيه.

يعني إحنا بننهي استخدام الـ Socket ده، ومش هنقدر نبدأ عليه عمليات جديدة وهو مقفول.

خلينا نشوف الفرق بينهم:

| المقارنة                      | `shutdown()`               | `close()`             |
| ----------------------------- | -------------------------- | --------------------- |
| الوظيفة الأساسية              | إيقاف الإرسال أو الاستقبال | إغلاق الـ Socket      |
| تحدد اتجاه الإغلاق؟           | أيوه                       | لأ                    |
| تحرر مورد الـ Socket؟         | مش ده هدفها                | أيوه                  |
| ممكن تبدأ عمليات جديدة بعدها؟ | يعتمد على الاتجاه والحالة  | لا، لازم Socket مفتوح |

### ليه بنستخدم الاتنين مع بعض؟

```
asio::error_code ignored_ec;

m_socket.shutdown(
    asio::ip::tcp::socket::shutdown_both,
    ignored_ec
);

m_socket.close(ignored_ec);
```

إحنا بنحاول نوقف الاتصال بشكل واضح، وبعدها نقفل الـ Socket نفسه.

لكن هل لازم دائمًا نستخدم `shutdown()` قبل `close()`؟

لأ. تقدر تقفل الـ Socket مباشرة باستخدام `close()`. وفي حالة إنهاء الاتصال بسبب خطأ أو انقطاع العميل، ده غالبًا كفاية لإغلاق الـ Socket وإلغاء عملياته المعلقة.

استخدام `shutdown()` قبل `close()` ممكن يكون مفيدًا لما يكون عندك سبب محدد للتحكم في إنهاء الاتصال، لكنه مش شرط عام.

## 4. إيه علاقة الكلام ده بالـ Asynchronous Operations؟

دي أهم نقطة في الكود بتاعنا.

تخيل إن الـ Server بدأ يستقبل بيانات من Client باستخدام:

```
m_socket.async_read_some(
    asio::buffer(m_buffer),
    [this](const asio::error_code& ec,
           std::size_t bytes_transferred)
    {
        if (ec) {
            return;
        }

        // Process received data
    }
);
```

إيه اللي حصل هنا؟

1. طلبنا من Asio بدء عملية قراءة.
2. العملية لسه ما خلصتش، فـ Asio احتفظ بالـ Completion Handler عشان يستدعيه لما العملية تكتمل.
3. الـ Server يقدر يكمل تنفيذ كود تاني بدل ما يستنى القراءة تنتهي.

دلوقتي افترض إننا قررنا نقفل الاتصال قبل ما القراءة تكتمل.

```
m_socket.close(ignored_ec);
```

في الحالة دي، إغلاق الـ Socket بيلغي العمليات غير المكتملة المرتبطة بيه عادةً، وبتوصل نتيجة الإلغاء للـ Completion Handler على هيئة:

```
asio::error::operation_aborted
```

لكن خلي بالك: الـ Handler مش بالضرورة بيتنفذ لحظة استدعاء `close()`. Asio عادةً بتسلمه للتنفيذ من خلال الـ `io_context`، وبالتالي لازم الـ Event Loop يواصل معالجة الـ Handlers.

ممكن تتخيل التسلسل كده:

`async_read_some()`

بدء عملية قراءة غير متزامنة

العملية لسه معلقة

الـ Completion Handler موجود ومستني النتيجة

`m_socket.close(ignored_ec)`

إغلاق الـ Socket وإلغاء العملية المعلقة

Completion Handler

يُنفّذ لاحقًا مع نتيجة الإلغاء، غالبًا `operation_aborted`

### طيب، ليه النقطة دي مهمة في الـ `Connection` class؟

لأن إغلاق الـ Socket مش معناه إن كائن الـ `Connection` نفسه اتدمر، ومش معناه إن كل الـ Handlers اللي بتشير إليه اختفت من الذاكرة.

مثلًا، لو عندك:

```
[this](const asio::error_code& ec,
       std::size_t bytes_transferred)
{
    // استخدام أعضاء الـ Connection
}
```

الـ `this` هنا مجرد مؤشر إلى الكائن. لو الكائن اتدمر قبل تنفيذ الـ Handler، ممكن تحصل مشكلة `use-after-free`.

عشان كده بنحتاج ندير عمر الـ Connection بشكل صحيح، وغالبًا بنستخدم `std::shared_ptr` و`std::enable_shared_from_this` حسب تصميم الكلاس، بحيث نضمن بقاء الكائن حيًا طالما العملية غير المتزامنة محتاجاه.

الخلاصة: `close()` بتنهي الـ Socket، لكن إدارة عمر الـ `Connection` مسؤولية منفصلة.

# الجزء الثاني: شرح `std::function` بالتفصيل

خلينا نبدأ بسؤال بسيط: إزاي نخزن Function داخل متغير؟

في C++، ممكن تكون متعود تكتب:

```
void say_hello()
{
    std::cout << "Hello\n";
}
```

وتستدعيها:

```
say_hello();
```

لكن ماذا لو عايز تخزن وظيفة معينة في متغير، وتقدر تغير الوظيفة دي لاحقًا، أو تبعتها لدالة تانية؟

هنا بييجي دور `std::function`.

## 1. يعني إيه `std::function`؟

`std::function` هو كلاس من مكتبة C++ القياسية، موجود في:

```
#include <functional>
```

بيوفر وسيلة لتخزين واستدعاء أي Callable Object مناسب لتوقيع معين.

كلمة Callable معناها أي كائن تقدر تستدعيه باستخدام الأقواس `()`، زي:

- Function عادية.
- Lambda Expression.
- Function Object (كائن فيه `operator()`).
- Member Function بعد ربطها بكائن باستخدام أدوات زي `std::bind` أو Lambda مناسبة.

خلينا نبدأ بمثال بسيط.

```
#include <iostream>
#include <functional>

void say_hello()
{
    std::cout << "Hello!\n";
}

int main()
{
    std::function<void()> callback = say_hello;

    callback();
}
```

الناتج:

```
Hello!
```

نفهم التعريف ده:

```
std::function<void()> callback;
```

الجزء `void()` معناه إننا عايزين نخزن Callable:

- بترجع `void`، يعني مفيش قيمة راجعة.
- ومش بتستقبل أي Parameters.

إذن `callback` مش Function جديدة كتبنا جسمها، لكنها كائن يقدر يحتفظ بعملية قابلة للاستدعاء بالتوقيع ده.

لاحظ إننا كتبنا:

```
callback = say_hello;
```

مش:

```
callback = say_hello();
```

الأولى بتخزن الدالة نفسها. الثانية بتحاول تنفذها فورًا.

## 2. إزاي نفهم `std::function<void()>`؟

خلينا نغيّر نوع الدالة شوية.

افترض عندنا Function بتستقبل رقمين وترجع رقمًا:

```
int add(int a, int b)
{
    return a + b;
}
```

نقدر نخزنها بالشكل ده:

```
std::function<int(int, int)> operation = add;

int result = operation(10, 20);

std::cout << result << '\n'; // 30
```

اقرأ التعريف بالشكل ده:

```
std::function<int(int, int)>
```

- `int` الأولى: نوع القيمة الراجعة.
- `int, int` داخل الأقواس: أنواع الـ Parameters.
- `operation`: المتغير اللي بيحتفظ بالـ Callable.

شوف أمثلة مختلفة:

| التعريف                            | معناه                                            |
| ---------------------------------- | ------------------------------------------------ |
| `std::function<void()>`            | بدون Parameters، ولا توجد قيمة راجعة             |
| `std::function<void(int)>`         | Parameter واحد من نوع `int`، ولا توجد قيمة راجعة |
| `std::function<int(int, int)>`     | Parameterان من نوع `int`، والقيمة الراجعة `int`  |
| `std::function<bool(std::string)>` | يستقبل `std::string` ويرجع `bool`                |

القاعدة المهمة: التوقيع (Signature) اللي بتحدده هو اللي بيحدد شكل الـ Callable اللي تقدر تخزنه.

## 3. هل `std::function` بيخزن Functions عادية بس؟

لأ، ودي من أهم مميزاته.

### المثال الأول: تخزين Lambda

الـ Lambda هي طريقة لتعريف Callable مباشرة في المكان اللي محتاجه فيه.

```
std::function<void()> callback = []()
{
    std::cout << "Hello from Lambda\n";
};

callback();
```

هنا إحنا ما كتبناش Function باسم معين. كتبنا Lambda وخزناها في `callback`.

### المثال الثاني: Lambda بتستقبل Parameters

```
std::function<void(int)> print_number =
    [](int number)
    {
        std::cout << number << '\n';
    };

print_number(42);
```

هنا التوقيع `void(int)` متوافق مع الـ Lambda لأنها بتستقبل `int` ومش بترجع قيمة.

### المثال الثالث: تخزين Lambda بتحتفظ بحالة (State)

بص على الكود ده:

```
int counter = 0;

std::function<void()> increment =
    [&counter]()
    {
        ++counter;
        std::cout << counter << '\n';
    };

increment(); // 1
increment(); // 2
increment(); // 3
```

إيه المختلف هنا؟

الـ Lambda بتستخدم:

```
[&counter]
```

وده معناه إنها بتحتفظ بمرجع للمتغير `counter` الموجود خارجها.

لما نستدعي `increment()`، بتزيد قيمة المتغير الأصلي.

لاحظ إن `std::function` هنا بيحتفظ بالـ Lambda نفسها، بما فيها الـ Capture اللي بتستخدمه.

تنبيه: لازم المتغير اللي بنحتفظ بمرجع ليه يفضل موجودًا طول فترة استخدام الـ Lambda؛ وإلا ممكن يحصل Undefined Behavior.

## 4. يعني إيه الـ Capture في الـ Lambda؟

بما إننا بنتكلم عن `std::function` في Asio، فلازم نفهم الـ Capture كويس.

شوف المثال ده:

```
int number = 10;

auto callback = [number]()
{
    std::cout << number << '\n';
};
```

لما كتبنا:

```
[number]
```

إحنا بنلتقط قيمة `number` بالنسخ (Capture by Value).

يعني الـ Lambda بتحتفظ بنسخة من القيمة، مش بمرجع للمتغير الأصلي.

أما هنا:

```
auto callback = [&number]()
{
    std::cout << number << '\n';
};
```

فإحنا بنلتقط المتغير بالمرجع (Capture by Reference).

الفرق مهم جدًا:

| Capture     | السلوك                                                               |
| ----------- | -------------------------------------------------------------------- |
| `[number]`  | الاحتفاظ بنسخة من القيمة                                             |
| `[&number]` | الاحتفاظ بمرجع للمتغير الأصلي                                        |
| `[this]`    | الاحتفاظ بمؤشر إلى الكائن الحالي                                     |
| `[=]`       | التقاط المتغيرات المستخدمة ضمنيًا بالنسخ، مع مراعاة قواعد الـ Lambda |
| `[&]`       | التقاط المتغيرات المستخدمة ضمنيًا بالمرجع                            |

في كود الـ Asio اللي عندنا، لو كتبنا:

```
[this](const asio::error_code& ec,
       std::size_t bytes_transferred)
{
    // ...
}
```

فالـ Lambda بتحتفظ بمؤشر `this`، مش بنسخة من كائن الـ Connection.

وده سبب إضافي يخلي إدارة عمر الكائن مهمة.

## 5. إيه علاقة `std::function` بالـ Completion Handler في Asio؟

دلوقتي نربط الكلام كله بمشروعنا.

لما بنكتب:

```
m_socket.async_read_some(
    asio::buffer(m_buffer),
    [this](const asio::error_code& ec,
           std::size_t bytes_transferred)
    {
        if (ec) {
            return;
        }

        // Process received data
    }
);
```

إحنا بنمرر Lambda إلى `async_read_some()`.

الـ Lambda دي هي الـ Completion Handler، يعني الوظيفة اللي Asio هتستدعيها لما عملية القراءة تكتمل أو تنتهي بخطأ.

طيب، هل لازم نكتب Lambda مباشرة؟ لأ. نقدر نعرّف نوعًا مناسبًا باستخدام `std::function`.

مثلًا:

```
using ReadHandler =
    std::function<void(
        const asio::error_code&,
        std::size_t
    )>;
```

ده معناه إن `ReadHandler` اسم بديل لنوع Callable:

- بيستقبل `const asio::error_code&`.
- بيستقبل `std::size_t`.
- بيرجع `void`.

نقدر نستخدمه بالشكل ده:

```
ReadHandler handler =
    [](const asio::error_code& ec,
       std::size_t bytes_transferred)
    {
        if (ec) {
            std::cout << ec.message() << '\n';
            return;
        }

        std::cout << "Received "
                  << bytes_transferred
                  << " bytes\n";
    };

handler(asio::error_code{}, 128);
```

هنا استدعينا الـ Handler يدويًا للتوضيح، ومررنا له نتيجة نجاح وعددًا افتراضيًا من البايتات.

لكن في Asio، إحنا عادةً بنمرر الـ Handler إلى العملية غير المتزامنة، وAsio هي اللي بتستدعيه عند اكتمال العملية.

### هل لازم أستخدم `std::function` مع كل Completion Handler؟

لأ، إطلاقًا.

في الاستخدام المباشر:

```
m_socket.async_read_some(
    asio::buffer(m_buffer),
    [this](const asio::error_code& ec,
           std::size_t bytes_transferred)
    {
        // ...
    }
);
```

مش محتاج تكتب `std::function` بنفسك. Asio تقدر تستقبل الـ Lambda مباشرةً وتستنتج نوعها.

أما `std::function` فبتكون مفيدة لما تحتاج تخزن Handler في متغير أو عضو في Class، أو تمرره بين أجزاء مختلفة من البرنامج من غير ما تعتمد على النوع الفعلي للـ Lambda.

## 6. استخدام `std::function` داخل Class

خلينا نعمل مثال قريب من تصميم الـ Server عندنا.

```
#include <functional>
#include <iostream>

class Connection
{
public:
    using MessageHandler =
        std::function<void(const std::string&)>;

    void set_message_handler(MessageHandler handler)
    {
        m_handler = std::move(handler);
    }

    void receive_message(const std::string& message)
    {
        if (m_handler) {
            m_handler(message);
        }
    }

private:
    MessageHandler m_handler;
};
```

نستخدم الكلاس كده:

```
int main()
{
    Connection connection;

    connection.set_message_handler(
        [](const std::string& message)
        {
            std::cout << "Received: "
                      << message << '\n';
        }
    );

    connection.receive_message("Hello Server");
}
```

الناتج:

```
Received: Hello Server
```

إيه اللي حصل؟

1. `MessageHandler` بيحدد نوع الـ Callback المقبول.
2. `set_message_handler()` بيخزن الـ Callback داخل `m_handler`.
3. لما توصل رسالة، `receive_message()` بتستدعي الـ Callback المخزن.
4. الـ Connection مش محتاج يعرف تفاصيل الوظيفة اللي هتتعامل مع الرسالة.

ده بيساعدنا نفصل بين استقبال الرسائل وطريقة التعامل معها.

لاحظ كمان إننا استخدمنا:

```
if (m_handler)
```

لأن `std::function` ممكن تكون فاضية، واستدعاء `std::function` فاضية بيرمي `std::bad_function_call`.

## 7. هل `std::function` ليها تكلفة أداء؟

أيوه، ممكن يكون ليها تكلفة إضافية مقارنةً باستدعاء Lambda مباشرةً، وده بسبب إنها بتوفر نوعًا موحدًا لتخزين أنواع مختلفة من الـ Callables.

على سبيل المثال:

```
auto callback = []()
{
    // ...
};
```

مقابل:

```
std::function<void()> callback = []()
{
    // ...
};
```

في الحالة الأولى، `auto` بيحتفظ بالنوع الحقيقي للـ Lambda، وهو نوع يولده المترجم.

في الحالة الثانية، `std::function` بيقدم واجهة موحدة لاستدعاء Callable قد يكون نوعه مختلفًا عن أي Callable آخر.

التكلفة المحتملة تشمل:

- تكلفة إضافية في الاستدعاء غير المباشر.
- تخزين حالة الـ Callable وإدارتها.
- تخصيص ذاكرة ديناميكية في بعض الحالات، حسب حجم الـ Callable والتنفيذ.

لكن مش معنى كده إن كل `std::function` بتعمل Dynamic Allocation، أو إنها دائمًا أبطأ بشكل ملحوظ. التفاصيل تعتمد على التنفيذ ونوع الـ Callable والمترجم.

في سيرفر Asio، غالبًا اختيار التصميم الصحيح وإدارة عمر الاتصالات والعمليات المعلقة أهم من محاولة إزالة كل `std::function` دون قياس.

## 8. إمتى أستخدم `std::function` وإمتى أستخدم `auto`؟

| الحالة                                                       | الاختيار المناسب غالبًا    |
| ------------------------------------------------------------ | -------------------------- |
| تعريف Lambda محلية واستدعاؤها                                | `auto`                     |
| تمرير Lambda مباشرةً إلى Asio                                | Lambda مباشرةً             |
| تخزين Callbacks بأنواع مختلفة تحت توقيع موحد                 | `std::function`            |
| تمرير Callable إلى Template تريد فيه الحفاظ على نوعه الأصلي  | Template parameter         |
| تخزين Handler كعضو في Class مع السماح بأنواع Callable مختلفة | `std::function` خيار مناسب |

## خلاصة الجزئين

- `shutdown()` بتتحكم في اتجاهات إرسال واستقبال البيانات على الـ Socket.
- `close()` بتغلق الـ Socket، وإغلاقه بيلغي عادةً العمليات غير المكتملة المرتبطة بيه.
- نتيجة الإلغاء ممكن تكون `operation_aborted`، والـ Completion Handler بيتنفذ من خلال آلية إكمال العمليات في Asio.
- إغلاق الـ Socket مش معناه إن كائن الـ Connection نفسه اتدمر بأمان.
- `std::function` بتخزن Callable تحت توقيع محدد، سواء كانت Function أو Lambda أو Function Object.
- الـ Completion Handler ممكن يكون Lambda مباشرةً؛ مش لازم تحوله إلى `std::function` إلا لو محتاج تخزنه أو تستخدم واجهة موحدة له.

## مصادر موثوقة للتعمق

1. [Boost.Asio — ](https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/basic_stream_socket/shutdown.html?utm_source=chatgpt.com)[`basic_stream_socket::shutdown`](https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/basic_stream_socket/shutdown.html?utm_source=chatgpt.com) — شرح إيقاف اتجاهات الاتصال.
2. [Boost.Asio — ](https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/basic_socket/close.html?utm_source=chatgpt.com)[`basic_socket::close`](https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/basic_socket/close.html?utm_source=chatgpt.com) — شرح إغلاق الـ Socket.
3. [Boost.Asio — ](https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/error__basic_errors.html?utm_source=chatgpt.com)[`error::operation_aborted`](https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/error__basic_errors.html?utm_source=chatgpt.com) — أخطاء العمليات، ومنها `operation_aborted`.
4. [cppreference — ](https://en.cppreference.com/w/cpp/utility/functional/function.html?utm_source=chatgpt.com)[`std::function`](https://en.cppreference.com/w/cpp/utility/functional/function.html?utm_source=chatgpt.com) — مرجع تفصيلي لأنواع الـ Callables، التخزين، والاستدعاء.
5. [cppreference — Lambda expressions](https://en.cppreference.com/w/cpp/language/lambda.html?utm_source=chatgpt.com) — شرح الـ Capture والـ Lambda بالتفصيل.
