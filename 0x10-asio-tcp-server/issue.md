# المشكلة كاملة: ليه الـ Connection بيموت

> Compile the code then RUN and use nc in three terminals (3 clients) to test and check the output in all 4 terminals.

## الفكرة الأساسية: Asio مش بيمسك الـ object بتاعك

لما تنادي `async_read_some`، Asio بيسجل العملية عند الـ OS وبيرجع **فورًا**. هو بيحتفظ بالـ **handler** (الـ lambda) بس، لكن مش بيحتفظ بالـ `Connection` نفسه. لو الـ handler بيشاور على الـ Connection بـ `this` خام، يبقى **مفيش حاجة بتضمن إن الـ Connection لسه عايش**.

يعني عمر الـ object لازم **إنت** تديره.

## الخطوات اللي حصلت عندك، واحدة واحدة

```cpp
void Server::accept()
{
    auto connection = std::make_shared<Connection>(m_io);   // (1) ref count = 1

    m_acceptor.async_accept(
        connection->socket(),
        [this, connection](const asio::error_code& ec)      // (2) ref count = 2
        {
            connection->start();                            // (4)
            accept();
        });
}   // (3) المتغير المحلي connection يخرج من الـ scope: ref count = 1
```

**(1)** `make_shared` بيعمل Connection، والـ count = 1.

**(2)** الـ lambda بتاخد نسخة، فالـ count = 2.

**(3)** `accept()` بترجع، والمتغير المحلي بيتدمر، فالـ count = 1. دلوقتي **الـ lambda لوحدها** هي اللي ماسكة الـ Connection.

**(4)** لما حد يتوصل، الـ handler بيشتغل وبينادي `start()`، وهي بتبدأ `async_read_some` بـ lambda فيها `this` بس (مش `shared_ptr`) وبترجع.

**(5)** الـ accept handler بيخلص، والـ lambda بتاعته **بتتدمر** بعد ما بتنفذ، فالـ count = 0.

**(6)** الـ Connection بيتدمر، وده بيدمر `m_socket`، وهو في destructor بتاعه بيقفل الـ socket ويلغي أي عملية pending.

**(7)** الـ read handler بيتنفذ بـ `operation_aborted`، وده اللي طبع `Read error: Operation aborted.`

## ده الرسم الزمني

```
الوقت ──────────────────────────────────────────────►

accept handler:  [start()]──[خلص]
                    │          │
                    │          └─► shared_ptr يتدمر → Connection يتدمر → socket يتقفل
                    │
read handler:       └─► async_read_some بدأت ...... [aborted] ◄─ اتنفذ بعد الموت
```

الـ read كان لسه مستني بيانات من الـ client، بس الـ Connection اتدمر قبل ما أي بيانات توصل.

## ليه ده خطير مش بس "رسالة error"؟

الـ read handler اتنفذ على object **اتدمر خلاص**. أنت محظوظ إن الكود بتاعك بيطبع `ec.message()` بس، وده مش بيلمس `this`. لكن لو الـ handler عمل أي حاجة زي:

```cpp
[this](auto ec, auto n) {
    if (ec) { std::cerr << ec.message(); }
    write(n);          // ← بيستخدم this الميت
}
```

ده **use-after-free** وبيبقى undefined behavior: ممكن crash، أو corruption، أو يشتغل بالصدفة.

## الحل: خلي كل عملية async تمسك الـ Connection

```cpp
class Connection : public std::enable_shared_from_this<Connection> { ... };

void Connection::read()
{
    auto self = shared_from_this();      // نسخة shared_ptr من نفسه
    m_socket.async_read_some(
        asio::buffer(m_buffer),
        [this, self](const asio::error_code& ec, std::size_t n)
        {
            // self موجودة في الـ lambda → الـ count ≥ 1 → الـ Connection عايش
            if (ec) return;
            write(n);
        });
}
```

دلوقتي:
- طول ما فيه عملية pending، الـ lambda بتاعتها ماسكة `self`
- لما العملية تخلص، الـ handler بيبدأ العملية اللي بعدها (فيها `self` جديدة) قبل ما يتدمر
- لما الـ client يقفل أو يحصل error، الـ handler مش بيبدأ عملية جديدة، فآخر `self` بيتدمر والـ Connection بيتنضف **لوحده**

يعني الـ Connection عمره = **من أول عملية لآخر عملية**، من غير ما حد يديره يدوي.

## الخلاصة في سطرين

- **المشكلة:** الـ Connection مفيش حد ماسكه بعد ما الـ accept handler يخلص، فاتدمر والـ socket اتقفل وهو لسه مستني.
- **الحل:** `enable_shared_from_this` + كل handler يمسك `self`، فالـ Connection يعيش طول ما فيه عمليات شغالة.
