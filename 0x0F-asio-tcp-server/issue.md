# 1. المشكلة الأساسية

افترض إن عندنا `Connection` بيمثل client واحد:

```cpp
class Connection
{
public:
    void start()
    {
        socket_.async_read_some(...);
    }

private:
    asio::ip::tcp::socket socket_;
};
```

والـ server عمل:

```cpp
Connection connection;
connection.start();
```

هنا فيه سؤال مهم جدًا:

> مين يضمن إن `connection` لسه موجود لما عملية `async_read_some()` تخلص؟

---

# 2. الـ asynchronous operation مش بتشتغل فورًا

لما نكتب:

```cpp
socket_.async_read_some(
    asio::buffer(buffer_),
    handler
);
```

إحنا **مش بنقول لـ Asio اقرأ البيانات الآن**.

إحنا بنقول:

> "Asio، ابدأ عملية قراءة، ولما العملية تخلص نادِ الـ handler."

يعني:

```text
async_read_some()
       │
       │ register operation
       ▼
   Asio / OS
       │
       │ ... wait ...
       │
       ▼
data arrives / client disconnects
       │
       ▼
   handler()
```

فالـ `handler` هيشتغل **بعد شوية**.

---

# 3. وهنا تظهر مشكلة الـ lifetime

شوف المثال ده:

```cpp
void acceptClient()
{
    Connection connection(socket);

    connection.start();

} // connection destroyed here
```

بعد خروجنا من:

```cpp
acceptClient()
```

الـ object:

```cpp
connection
```

اتدمر.

لكن إحنا كنا عاملين:

```cpp
connection.start();
```

والـ `start()` عمل:

```cpp
socket_.async_read_some(...);
```

فالـ asynchronous operation لسه ممكن تخلص في المستقبل.

يبقى عندنا:

```text
Connection
    │
    ├── socket
    │
    └── async_read_some()
             │
             │
             ▼
          future
             
Connection destroyed ❌
```

وبعدها:

```text
async operation finishes
          │
          ▼
       handler
```

لكن الـ `Connection` اللي الـ handler محتاجه **مش موجود**.

---

# 4. طب نعملها global؟

ممكن حد يقول:

```cpp
Connection connection;
```

لكن طبعًا ده مش حل.

لأن عندنا:

```text
Client 1
Client 2
Client 3
Client 4
...
Client N
```

وكل client محتاج:

```cpp
Connection
```

فإحنا محتاجين طريقة منظمة لإدارة الـ lifetime.

---

# 5. وهنا يأتي `std::shared_ptr`

`shared_ptr` معناه ببساطة:

> أكثر من طرف ممكن يمتلك نفس object، والـ object يتدمر عندما يختفي آخر owner.

مثال:

```cpp
auto connection = std::make_shared<Connection>();
```

دلوقتي:

```text
shared_ptr
    │
    ▼
 Connection
```

لو عملنا:

```cpp
auto connection2 = connection;
```

يبقى:

```text
connection ───┐
              │
              ▼
          Connection
              ▲
              │
connection2 ──┘
```

الـ object عنده:

```text
2 owners
```

لو:

```cpp
connection.reset();
```

يبقى:

```text
connection2
     │
     ▼
 Connection
```

لسه موجود.

ولما:

```cpp
connection2.reset();
```

آخر owner اختفى، وبالتالي:

```text
Connection destroyed
```

وده بالضبط اللي نحتاجه في الـ asynchronous programming.

---

# 6. لكن فيه مشكلة ثانية

نفترض إن عندنا:

```cpp
class Connection
{
public:

    void start()
    {
        socket_.async_read_some(
            asio::buffer(buffer_),
            [](const asio::error_code& ec, std::size_t bytes)
            {
                // ...
            }
        );
    }

private:
    asio::ip::tcp::socket socket_;
    char buffer_[1024];
};
```

إحنا محتاجين الـ handler يكون قادر يحتفظ بالـ `Connection`.

يعني نريد شيئًا مثل:

```text
Connection
   │
   └── async operation
          │
          └── handler
                 │
                 └── shared_ptr<Connection>
```

وبالتالي:

```text
async operation
      │
      ▼
 handler owns Connection
      │
      ▼
Connection cannot die
until handler is finished
```

لكن هنا تظهر مشكلة:

> إزاي الـ object نفسه يحصل على `shared_ptr` يشير إلى نفسه؟

وهنا يأتي:

# `std::enable_shared_from_this`

---

# 7. `enable_shared_from_this`

بدل:

```cpp
class Connection
{
};
```

نكتب:

```cpp
class Connection
    : public std::enable_shared_from_this<Connection>
{
};
```

دلوقتي الـ `Connection` يقدر يعمل:

```cpp
shared_from_this()
```

والنتيجة:

```cpp
std::shared_ptr<Connection>
```

يشير إلى **نفس الـ object الحالي**.

---

# 8. مثال

```cpp
class Connection
    : public std::enable_shared_from_this<Connection>
{
public:

    void start()
    {
        auto self = shared_from_this();

        socket_.async_read_some(
            asio::buffer(buffer_),
            [self](const asio::error_code& ec,
                   std::size_t bytes)
            {
                // self موجود هنا
            }
        );
    }

private:

    asio::ip::tcp::socket socket_;
    char buffer_[1024];
};
```

ركز في السطر ده:

```cpp
auto self = shared_from_this();
```

هنا حصل:

```text
this
 │
 ▼
Connection object
 │
 │ shared_from_this()
 ▼
shared_ptr<Connection>
```

---

# 9. والـ lambda بتعمل إيه؟

شوف:

```cpp
[self](...)
{
    ...
}
```

`self` هنا بيتعمل له **copy** داخل الـ lambda.

يعني:

```text
Connection
    ▲
    │
    │ shared_ptr
    │
 async_read handler
```

فالـ handler أصبح owner للـ `Connection`.

---

# 10. وده يحل المشكلة

افترض إننا عملنا:

```cpp
auto connection = std::make_shared<Connection>();
connection->start();
```

بعدها:

```cpp
connection.reset();
```

هل الـ `Connection` هيتدمر؟

**لا.**

ليه؟

لأن الـ async handler عنده:

```cpp
self
```

يعني:

```text
main/server
    │
    │ shared_ptr
    ▼
Connection

async handler
    │
    │ shared_ptr
    ▼
Connection
```

لما الـ server يعمل:

```cpp
connection.reset();
```

يبقى:

```text
server owner
     X

async handler
     │
     ▼
Connection
```

فالـ object لسه موجود.

---

# 11. لما الـ client يفصل

دي النقطة اللي في النص اللي إنت بعتهالك.

الـ client يعمل:

```text
Client
   │
   X
disconnect
```

الـ OS / Asio يعرف إن الـ connection انتهت.

فالـ:

```cpp
async_read_some()
```

تخلص، والـ handler يتنفذ مثلاً:

```cpp
[self](const asio::error_code& ec,
       std::size_t bytes)
{
    if (ec)
    {
        std::cout << ec.message() << '\n';
        return;
    }
}
```

غالبًا هتشوف error مثل:

```text
End of file
```

أو:

```cpp
asio::error::eof
```

حسب الحالة وطريقة إغلاق الاتصال.

وده **مش bug**.

ده معناه:

> الـ asynchronous read انتهت لأن الـ peer أغلق الاتصال.

Asio توثق `eof` كحالة انتهاء الإدخال بسبب إغلاق الطرف الآخر للاتصال.

---

# 12. طيب مين يدمر الـ Connection؟

دي أهم نقطة.

مش لازم الـ handler يعمل:

```cpp
delete this;
```

❌ لا.

ومش لازم نستخدم:

```cpp
new Connection
```

وبعدين نفكر مين يعمل:

```cpp
delete
```

❌ ده بالضبط النوع من lifetime management اللي `shared_ptr` بيساعدنا نتجنبه.

بدل كده:

```cpp
std::shared_ptr<Connection>
```

يدير الـ lifetime.

الـ object يتدمر عندما **آخر `shared_ptr` يشير إليه يختفي**.

---

# 13. تخيل السيناريو كاملًا

عندنا:

```text
                    Server
                      │
                      │ shared_ptr
                      ▼
                ┌────────────┐
                │ Connection │
                └────────────┘
                      ▲
                      │
                 shared_ptr
                      │
                      │
              async_read handler
```

أثناء الـ async operation:

```text
reference count = 2
```

مثلًا:

```text
Server        → Connection
Handler       → Connection
```

بعد كده الـ server مش محتاج يحتفظ بالـ connection:

```cpp
connection.reset();
```

يبقى:

```text
reference count = 1

Handler → Connection
```

لما الـ handler يخلص:

```text
handler destroyed
```

يبقى:

```text
reference count = 0
```

فتحدث:

```text
Connection destroyed
```

وده هو الـ lifetime management اللي بنحتاجه.

---

# 14. طيب إيه علاقة ده بـ multiple clients؟

هنا تبدأ الصورة الكبيرة.

بدل ما يكون عندنا:

```text
Server
   │
   └── Client
```

هنبقى عندنا:

```text
                 Server
                   │
        ┌──────────┼──────────┐
        │          │          │
        ▼          ▼          ▼
   Connection  Connection  Connection
      │            │           │
      ▼            ▼           ▼
   Client 1     Client 2    Client 3
```

وكل واحد:

```cpp
std::shared_ptr<Connection>
```

مثلاً:

```cpp
auto connection =
    std::make_shared<Connection>(std::move(socket));

connection->start();
```

والـ server ممكن يحتفظ بالـ connections في container:

```cpp
std::vector<std::shared_ptr<Connection>>
```

أو تصميم أفضل حسب طريقة إدارة الـ sessions.

---

# 15. لكن خد بالك من نقطة مهمة جدًا

`shared_ptr` **مش معناه**:

> "أي object asynchronous لازم أستخدم له shared_ptr."

لا.

إحنا بنستخدمه هنا لأننا اخترنا design يكون فيه:

```text
Connection lifetime
        +
asynchronous operations
        +
multiple owners
```

والـ `enable_shared_from_this` بيسمح للـ object إنه ينشئ `shared_ptr` لنفسه **بشرط مهم جدًا**:

لازم يكون الـ object أصلًا مملوكًا بواسطة `shared_ptr`.

يعني ده صحيح:

```cpp
auto connection =
    std::make_shared<Connection>();

connection->start();
```

لكن ده خطير/غير صحيح:

```cpp
Connection connection;

connection.start();
```

ثم داخل `start()`:

```cpp
shared_from_this();
```

لأن الـ object لم يتم إنشاؤه/امتلاكه من خلال `shared_ptr`، وبالتالي `shared_from_this()` لا يملك control block صالحًا له. في هذه الحالة يمكن أن يؤدي إلى `std::bad_weak_ptr`.

---

# 16. الـ mental model اللي عايزك تثبته

احفظ الثلاثة دول مع بعض:

```text
async operation
      │
      │ needs object to stay alive
      ▼
 shared_ptr
      │
      │ object wants a shared_ptr to itself
      ▼
enable_shared_from_this
```

وبشكل أكثر تحديدًا:

```text
std::make_shared<Connection>()
             │
             ▼
       Connection object
             │
             │ shared_from_this()
             ▼
      shared_ptr<Connection>
             │
             ▼
       async handler
             │
             │ keeps object alive
             ▼
       handler finishes
             │
             ▼
     last shared_ptr dies
             │
             ▼
    Connection destroyed
```

---

## وده هيكون أساس الخطوة الجاية

الآن عندنا كل قطعة لوحدها:

```text
async_accept()
       │
       ▼
   new client
       │
       ▼
std::shared_ptr<Connection>
       │
       ▼
connection->start()
       │
       ▼
async_read_some()
       │
       ▼
shared_from_this()
       │
       ▼
handler
```

**الخطوة المنطقية التالية** هي إننا نبني `Connection` class كاملة ونعمل **TCP server يقبل Multiple Clients**، بحيث كل client له `Connection` مستقلة، ونشوف بالضبط `shared_ptr` و`shared_from_this()` بيتحركوا فين في الكود.
