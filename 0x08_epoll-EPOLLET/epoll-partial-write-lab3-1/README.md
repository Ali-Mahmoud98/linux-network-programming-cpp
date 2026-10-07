# epoll echo server

إحنا كده وصلنا لنقطة مهمة جدًا في `epoll`: **إزاي نتعامل مع `send()` لما مش كل الـ data تتبعت مرة واحدة**.

الخطوة الجاية هنثبت فيها مفهوم **Output Buffer + Partial Write** عمليًا، وبعدها ندمجه مع `EPOLLOUT`.

هنمشي كده:

```text
EPOLLIN
   │
   ▼
recv()
   │
   ▼
Application
   │
   ▼
outputBuffer
   │
   ▼
EPOLLOUT
   │
   ▼
send()
   │
   ├── all sent
   │      └── EPOLLOUT OFF
   │
   ├── partial
   │      └── update offset
   │
   └── EAGAIN
          └── wait for EPOLLOUT
```

### أول خطوة: نفهم `outputOffset`

لو عندنا:

```cpp
std::string outputBuffer = "Hello World";
std::size_t outputOffset = 0;
```

وحصل:

```cpp
send(fd, outputBuffer.data(), outputBuffer.size(), 0);
```

وافترضنا أن `send()` رجعت:

```text
5
```

يبقى:

```text
outputBuffer:

H e l l o   W o r l d
^
offset = 0

        send() → 5 bytes

H e l l o |   W o r l d
          ^
          offset = 5
```

إذن:

```cpp
outputOffset += bytesSent;
```

فتصبح:

```cpp
outputOffset = 5;
```

والـ send التالية لازم تبدأ من:

```cpp
outputBuffer.data() + outputOffset
```

يعني:

```cpp
send(
    fd,
    outputBuffer.data() + outputOffset,
    outputBuffer.size() - outputOffset,
    0
);
```

فتبعت:

```text
" World"
```

وليس `"Hello World"` مرة أخرى.

---

## التجربة اللي جاية

هنعمل `ClientConnection` بالشكل ده:

```cpp
struct ClientConnection
{
    int fd;

    std::string outputBuffer;
    std::size_t outputOffset = 0;
};
```

ونخلي السيرفر لما يستقبل:

```text
Hello
```

يضيف إلى:

```cpp
outputBuffer
```

مثلاً:

```text
Client
   │
   │ "Hello"
   ▼
recv()
   │
   ▼
outputBuffer

┌─────────────────────┐
│ H e l l o           │
└─────────────────────┘
          │
          ▼
       EPOLLOUT
          │
          ▼
        send()
```

والأهم أننا **لن نفترض أن `send()` تبعت كل حاجة**.

هنكتب `handleWrite()` بالشكل الصحيح:

```cpp
void handleWrite(ClientConnection& client)
{
    while (client.outputOffset < client.outputBuffer.size())
    {
        ssize_t bytesSent = send(
            client.fd,
            client.outputBuffer.data() + client.outputOffset,
            client.outputBuffer.size() - client.outputOffset,
            0
        );

        if (bytesSent > 0)
        {
            client.outputOffset += bytesSent;
            continue;
        }

        if (bytesSent == -1 &&
            (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            break;
        }

        // Real error
        break;
    }

    if (client.outputOffset == client.outputBuffer.size())
    {
        client.outputBuffer.clear();
        client.outputOffset = 0;

        // هنا نعمل EPOLLOUT OFF
    }
}
```

لاحظ الـ mental model هنا:

```text
outputBuffer
     │
     │
     ▼
┌─────────────────────────────┐
│ already sent │ not sent     │
└──────────────┴──────────────┘
               ▲
               │
          outputOffset
```

كل مرة `send()` تنجح:

```text
outputOffset += bytesSent
```

وكل مرة `send()` تعمل:

```text
EAGAIN
```

نقف وننتظر:

```text
EPOLLOUT
```

ثم نكمل من نفس الـ `outputOffset`.

**وده هو الأساس الحقيقي لأي asynchronous TCP server**؛ بعد كده هنستخدم نفس الفكرة في Asio لكن بدل ما ندير `EPOLLOUT` و`send()` يدويًا، Asio هيدير الـ readiness والـ async write عننا.
