# NOTES ON EPOLLIN and EPOLLET - Level-Triggered and Edge-Triggered

أيوه، دي فعلًا نقطة محتاجة تتشرح بهدوء، لأن عبارة **"drain until EAGAIN"** لو اتقالت لوحدها ممكن تكون confusing.

خلينا ننسى الكود لمدة دقيقة ونبني الموضوع من الصفر.

---

## 1. أولًا: إيه اللي `EPOLLIN` بيقولك؟

لما `epoll_wait()` يرجع:

```cpp
EPOLLIN
```

فهو **مش بيقولك**:

> "فيه رسالة واحدة جاهزة."

ومش بيقول:

> "اقرأ 1024 bytes."

هو بيقول بشكل أبسط:

> **"الـ file descriptor ده حاليًا عنده data ممكن تقرأها بدون ما `recv()` تعمل blocking."**

دي نقطة مهمة جدًا.

---

## 2. تخيل الـ socket كأنه صندوق

عندنا client:

```text
Client
   │
   │ send("HELLO")
   ▼
┌──────────────────────┐
│ Kernel Receive Buffer │
│                      │
│ H E L L O            │
└──────────────────────┘
```

الـ kernel حاطط البيانات في **receive buffer** الخاص بالـ socket.

لما يكون فيه data:

```text
Receive Buffer
┌──────────────────────┐
│ H E L L O            │
└──────────────────────┘
          │
          ▼
       readable
          │
          ▼
       EPOLLIN
```

---

## 3. طيب `recv()` بتعمل إيه؟

لما نعمل:

```cpp
recv(fd, buffer, 1024, 0);
```

إحنا بنقول للـ kernel:

> "هاتلي up to 1024 bytes من البيانات الموجودة في receive buffer."

لاحظ كلمة:

**up to**

مش معناها لازم يرجع 1024.

ممكن يرجع:

```text
1024
500
100
10
1
```

حسب البيانات المتاحة والظروف.

---

## 4. مثال صغير جدًا

افترض إن الـ receive buffer فيه:

```text
┌─────────────────────┐
│ ABCDEFGHIJ           │
└─────────────────────┘
```

يعني:

```text
10 bytes
```

وإحنا عملنا:

```cpp
char buffer[4];

recv(fd, buffer, 4, 0);
```

ممكن نقرأ:

```text
ABCD
```

ويبقى في الـ kernel:

```text
┌─────────────────────┐
│ EFGHIJ               │
└─────────────────────┘
```

لاحظ:

**الـ socket لسه readable.**

ليه؟

لأن:

```text
EFGHIJ
```

لسه موجودة.

---

## 5. هنا بالضبط نفهم LT

في **Level-Triggered**:

```text
Data exists
    │
    ▼
EPOLLIN
    │
    ▼
recv()
    │
    ▼
Data still exists
    │
    ▼
EPOLLIN again
```

يعني لو رجعت للـ event loop:

```cpp
epoll_wait();
```

ممكن `epoll` يقولك تاني:

```text
EPOLLIN
```

لأن الـ condition لسه موجودة.

---

## 6. لكن مع Edge-Triggered؟

هنا الموضوع مختلف.

خلينا نفترض إن الـ socket كان:

```text
EMPTY
```

وبعدين client بعت:

```text
HELLO
```

حصل transition:

```text
EMPTY
  │
  │ data arrives
  ▼
DATA AVAILABLE
```

وده هو الـ **edge**.

فتاخد:

```text
EPOLLIN
```

---

## 7. وهنا المشكلة

أنت أخذت:

```text
EPOLLIN
```

وعملت:

```cpp
recv(fd, buffer, 4, 0);
```

وقريت:

```text
HELL
```

لكن:

```text
O
```

لسه في الـ kernel.

يعني:

```text
Kernel receive buffer:

┌──────────────────────┐
│ O                    │
└──────────────────────┘
```

الـ socket ما زال readable.

لكن إحنا في **Edge-Triggered**.

والـ edge الذي حدث كان:

```text
EMPTY
   │
   │ data arrived
   ▼
READY
```

إحنا بالفعل اتبلغنا عن هذا الانتقال.

---

## 8. لذلك لازم نعمل إيه؟

لما يوصلك:

```text
EPOLLIN
```

مع `EPOLLET`، تقول:

> "أنا هقرأ **كل البيانات المتاحة حاليًا** لحد ما kernel يقول لي مفيش أكتر."

يعني:

```cpp
while (true)
{
    recv(...);
}
```

لكن طبعًا مش هنفضل للأبد.

لما مفيش data:

```cpp
recv(...)
```

هيرجع:

```text
-1
```

و:

```cpp
errno == EAGAIN
```

وهنا نعرف:

> خلاص، استنزفنا الـ socket.

---

# 9. تعال نمشي على مثال كامل

افترض إن client أرسل:

```text
3000 bytes
```

والـ buffer بتاعنا:

```cpp
char buffer[1024];
```

يبقى ممكن يحصل:

```text
                 Client
                    │
                    │ 3000 bytes
                    ▼
          ┌────────────────────┐
          │ Receive Buffer     │
          │ 3000 bytes         │
          └─────────┬──────────┘
                    │
                    ▼
                 EPOLLIN
                    │
                    ▼
                recv(1024)
                    │
                    ▼
              1976 remain
                    │
                    ▼
                recv(1024)
                    │
                    ▼
               952 remain
                    │
                    ▼
                recv(952)
                    │
                    ▼
                0 remain
                    │
                    ▼
                recv()
                    │
                    ▼
                EAGAIN
```

هنا نقول:

```text
DONE
```

---

## 10. ليه `EAGAIN` هو نقطة التوقف؟

لأننا بنستخدم **non-blocking socket**.

وده معناه:

لو مفيش data حاليًا:

```cpp
recv(fd, buffer, sizeof(buffer), 0);
```

مش هيعمل:

```text
BLOCK
```

لكن هيقول:

```text
EAGAIN
```

يعني:

> "مفيش حاجة أقدر أقرأها دلوقتي بدون انتظار."

وده بالضبط اللي إحنا عايزينه.

---

## 11. إذًا `EAGAIN` مش failure هنا

دي نقطة مهمة جدًا.

لما تشوف:

```cpp
if (errno == EAGAIN ||
    errno == EWOULDBLOCK)
{
    break;
}
```

ما تفسرهاش:

> حصل error.

فسرها:

> **خلصنا كل الـ data الموجودة حاليًا.**

يعني:

```text
recv()
  │
  ├── data → process it
  │
  ├── data → process it
  │
  ├── data → process it
  │
  └── EAGAIN
           │
           ▼
       nothing left
           │
           ▼
          STOP
```

---

## 12. طيب ليه مينفعش أعمل `recv()` مرة واحدة؟

ده السؤال الأهم.

لو عملنا:

```cpp
if (events[i].events & EPOLLIN)
{
    recv(fd, buffer, sizeof(buffer), 0);
}
```

فممكن السيناريو يكون:

```text
Client sends 3000 bytes
        │
        ▼
     EPOLLIN
        │
        ▼
   recv() → 1024
        │
        ▼
   return to epoll_wait()
```

ويبقى:

```text
1976 bytes
```

لسه في الـ socket.

في **Level-Triggered**، غالبًا مش مشكلة:

```text
epoll_wait()
     │
     ▼
EPOLLIN again
```

فتقرأ الباقي.

لكن في **Edge-Triggered** أنت بتبني الـ logic على الـ **edge**، وليس على استمرار الـ level.

لذلك القاعدة العملية:

```text
EPOLLIN + EPOLLET
        │
        ▼
read/recv repeatedly
        │
        ▼
     EAGAIN
        │
        ▼
       stop
```

---

## 13. خلي بالك من نقطة دقيقة جدًا

مش معنى الكلام ده إن:

```text
EPOLLET
```

يضمن لك حرفيًا:

> "لن يأتي أي event آخر طالما فيه data."

المهم هو إنك **لا تعتمد على event جديد** لكي تكتشف البيانات التي تركتها.

أنت كـ programmer مسؤول عن استنزاف الـ FD عندما يصلك الـ edge.

ولهذا Linux man page نفسها توصي، عند استخدام edge-triggered mode، باستخدام nonblocking file descriptors والقراءة حتى `EAGAIN`.

---

## 14. نفس الفكرة مع `accept()`

ودي بتخلي الموضوع كله logical جدًا.

تخيل عندنا:

```text
3 clients connect
```

فالـ listening socket عنده:

```text
pending connections:

┌──────────────┐
│ Client A     │
│ Client B     │
│ Client C     │
└──────────────┘
```

وصل:

```text
EPOLLIN
```

لو عملت:

```cpp
accept();
```

مرة واحدة:

```text
Client A
```

يبقى:

```text
Client B
Client C
```

لسه موجودين.

لذلك:

```cpp
while (true)
{
    int clientFd = accept(...);

    if (clientFd == -1)
    {
        if (errno == EAGAIN)
            break;
    }

    // handle client
}
```

فتصبح:

```text
EPOLLIN
   │
   ▼
accept() → A
   │
   ▼
accept() → B
   │
   ▼
accept() → C
   │
   ▼
accept() → EAGAIN
   │
   ▼
DONE
```

---

## 15. إذًا عندنا قاعدة واحدة تنطبق على كل I/O

وده أهم شيء عايزك تخرج بيه.

في `EPOLLET`:

```text
             Event
               │
               ▼
        ┌──────────────┐
        │ Do I/O       │
        └──────┬───────┘
               │
               ▼
       Is it EAGAIN?
          /       \
        No         Yes
        │           │
        ▼           ▼
     continue      STOP
        │
        └───────►
```

يعني:

### `accept()`

```text
accept()
accept()
accept()
EAGAIN → stop
```

### `recv()`

```text
recv()
recv()
recv()
EAGAIN → stop
```

### `send()`

```text
send()
send()
send()
EAGAIN → stop
```

ودي نقطة جميلة جدًا لأنك بدأت تشوف إن:

> **`EAGAIN` هو جزء أساسي من الـ event-driven programming، وليس مجرد error handling.**

---

## 16. وده يفسر الـ Rule اللي قلتها لك

لما قلت:

```text
EPOLLET
   │
   ▼
Handle the edge
   │
   ▼
drain work
   │
   ▼
EAGAIN
   │
   ▼
STOP
```

المقصود بـ **drain** هو:

> **نفّذ عملية الـ I/O المتاحة حاليًا لحد ما الـ nonblocking FD يقول لك: مفيش أكتر.**

مش معناها "امسح buffer" أو "اقرأ packet واحدة".

---

## وأخيرًا: مقارنة تحفظها

```text
LEVEL-TRIGGERED
────────────────────────────

data available
      │
      ▼
  EPOLLIN
      │
    recv()
      │
      ▼
data still available
      │
      ▼
  EPOLLIN again
```

ممكن تقرأ **جزء** وترجع للـ event loop.

أما:

```text
EDGE-TRIGGERED
────────────────────────────

data arrives
      │
      ▼
    EPOLLIN
      │
      ▼
   recv()
      │
      ▼
   recv()
      │
      ▼
   recv()
      │
      ▼
  EAGAIN
      │
      ▼
     STOP
```

لا تعتمد على notification جديد لكي يخبرك بالـ data التي تركتها.

**الخطوة العملية الأفضل الآن** هي تجربة صغيرة جدًا بدل السيرفر الكبير: نعمل `EPOLLET` مع client واحد، ونضع `recv()` مرة واحدة، ثم نضع `recv()` داخل `while` ونشوف الفرق بأنفسنا. بعدها فقط نرجع للـ TCP server الكامل.
