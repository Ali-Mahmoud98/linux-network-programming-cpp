# `POLLOUT` and Partial Writes

Let's take a small step back before `epoll()` and understand one of the most important points in **non-blocking networking**:

> `POLLOUT` + `partial write`

This will matter a lot later, when we move to `epoll` and async networking.

## 1. What does `POLLOUT` mean?

We already know:

```cpp
POLLIN
```

It roughly means:

> The socket can `read/recv` right now without waiting.

And:

```cpp
POLLOUT
```

means:

> The socket has space available in its **send buffer**, so a `write/send` operation can make progress without blocking.

The picture:

```text
Application
     │
     │ send()
     ▼
┌───────────────┐
│ Kernel Socket │
│ Send Buffer   │
└───────┬───────┘
        │
        │ TCP
        ▼
      Network
```

If the send buffer is full:

```text
send()
  │
  ▼
Send Buffer
████████████████
      FULL
        │
        ▼
   EAGAIN / EWOULDBLOCK
```

If there is space:

```text
Send Buffer
████████░░░░░░░░
         ↑
      available
```

then the socket is considered **writable**.

---

## 2. Very important: `POLLOUT` does not mean the other side read the data

This point needs to stick.

```text
POLLOUT
   ≠
"client received my data"
```

It roughly means:

```text
POLLOUT
   ↓
Kernel send buffer
has available space
   ↓
send() can make progress
```

---

## 3. Why do we need `POLLOUT`?

With a blocking socket:

```cpp
send(fd, buffer, size, 0);
```

`send()` may wait if the send buffer is full.

But we are working with:

```text
NON-BLOCKING
```

sockets, so instead of letting the application stall, we do this:

```text
send()
  │
  ├── possible now
  │      ↓
  │    send
  │
  └── not possible now
         ↓
      EAGAIN
```

In the second case, we wait for:

```text
POLLOUT
```

---

## 4. But here an important problem appears: Partial Write

Suppose we have:

```cpp
char buffer[10000];
```

and we want to send:

```text
10000 bytes
```

We might call:

```cpp
ssize_t n = send(
    socket_fd,
    buffer,
    10000,
    0);
```

and expect:

```text
n == 10000
```

But that's **not guaranteed**.

It could be:

```text
n == 4096
```

or:

```text
n == 2048
```

or any positive value less than `10000`.

That is:

```text
Application wants:

[====================] 10000 bytes

             send()
               │
               ▼

Kernel accepted:

[========] 4096 bytes

Remaining:

        [============] 5904 bytes
```

This is called a:

> **Partial Write**

---

## 5. Why does this happen?

Because a non-blocking socket has a limited buffer.

For example:

```text
Application
     │
     │ 10000 bytes
     ▼
┌──────────────────┐
│ Socket Send      │
│ Buffer           │
│                  │
│ [████████░░░░░░] │
└──────────────────┘
```

The available space might only be `4096` bytes.

So the kernel says:

> I can take 4096 right now.

and returns:

```cpp
4096
```

instead of:

```cpp
10000
```

---

## 6. So what do we do with the remaining 5904 bytes?

**We keep them.**

That's the application's responsibility.

For example:

```cpp
struct SendBuffer
{
    std::vector<char> data;
    std::size_t offset = 0;
};
```

After the first send:

```text
data:

[====================]
       10000 bytes

offset:
    ↑
   4096
```

Meaning:

```text
sent:

[========]

remaining:

        [============]
          5904 bytes
```

Then we wait for:

```text
POLLOUT
```

and when it fires, we call:

```cpp
send(...)
```

again, starting from:

```cpp
data.data() + offset
```

---

## 7. A very small example

Let's write a helper:

```cpp
ssize_t sendNonBlocking(
    int fd,
    const char* data,
    std::size_t size)
{
    return send(
        fd,
        data,
        size,
        0);
}
```

But the caller must handle the result:

```cpp
ssize_t n = send(
    fd,
    buffer,
    size,
    0);

if (n > 0)
{
    // n bytes were sent
}
else if (n == -1)
{
    if (errno == EAGAIN ||
        errno == EWOULDBLOCK)
    {
        // Try again when POLLOUT occurs
    }
}
```

Notice:

```text
send()
 │
 ├── n > 0
 │      │
 │      └── progress happened
 │
 └── EAGAIN
        │
        └── wait for POLLOUT
```

---

## 8. Now let's connect it to `poll()`

We have:

```cpp
pollfd pfd{};
```

To watch for reads:

```cpp
pfd.events = POLLIN;
```

To watch for writes:

```cpp
pfd.events = POLLOUT;
```

And for both:

```cpp
pfd.events = POLLIN | POLLOUT;
```

For example:

```text
┌─────────────────────┐
│       pollfd        │
├─────────────────────┤
│ fd = 5              │
│                     │
│ events:             │
│   POLLIN            │
│   POLLOUT           │
└─────────────────────┘
```

After:

```cpp
poll(...)
```

we check:

```cpp
if (pfd.revents & POLLOUT)
{
    // socket can currently accept more outgoing data
}
```

---

## 9. But there's a very common mistake here

Don't do this:

```cpp
pfd.events = POLLIN | POLLOUT;
```

**for every socket, all the time.**

Why?

Because most TCP sockets are writable most of the time.

Imagine:

```text
poll()

fd 4 → POLLOUT
fd 5 → POLLOUT
fd 6 → POLLOUT
fd 7 → POLLOUT
fd 8 → POLLOUT
```

even though we have no data to send!

The event loop will keep waking up:

```text
poll()
  ↓
POLLOUT
  ↓
poll()
  ↓
POLLOUT
  ↓
poll()
  ↓
POLLOUT
```

This can cause a **busy loop / unnecessary wakeups**.

---

## 10. The correct approach

We watch for `POLLOUT` **only when we have pending data**.

For example:

```text
No pending data
       │
       ▼
events = POLLIN
```

If the application wants to send something:

```text
Pending data exists
       │
       ▼
events = POLLIN | POLLOUT
```

After all the data has been sent:

```text
Send buffer empty
       │
       ▼
remove POLLOUT
       │
       ▼
events = POLLIN
```

This is a very important concept:

```text
           Client Socket
                │
        ┌───────┴───────┐
        │               │
   Read handling    Write handling
        │               │
     POLLIN          POLLOUT
                        │
                        ▼
                 Pending buffer
```

---

## 11. Adding it to the TCP Server

Our server currently works like this:

```text
Client
  │
  │ request
  ▼
recv()
  │
  ▼
process
  │
  ▼
send()
```

But the production-style model looks like this:

```text
Client
  │
  │ data
  ▼
POLLIN
  │
  ▼
recv()
  │
  ▼
Outgoing Buffer
  │
  │ pending data
  ▼
Enable POLLOUT
  │
  ▼
poll()
  │
  ▼
POLLOUT
  │
  ▼
send()
  │
  ├── all sent
  │      ↓
  │   disable POLLOUT
  │
  └── partial
         ↓
      keep buffer
         ↓
      wait POLLOUT
```

This is roughly the **state machine** we need:

```text
             ┌──────────────┐
             │   Connected  │
             └──────┬───────┘
                    │
                 POLLIN
                    │
                    ▼
             ┌──────────────┐
             │    Reading   │
             └──────┬───────┘
                    │
              response ready
                    │
                    ▼
             ┌──────────────┐
             │ Writing Data │
             └──────┬───────┘
                    │
             ┌──────┴───────┐
             ▼              ▼
        all sent         partial
             │              │
             ▼              ▼
        POLLIN only      POLLOUT
                            │
                            └───────► Writing
```

---

## 12. The 4 most important ideas to take with you

```text
1. POLLIN
   ↓
   recv/read can make progress


2. POLLOUT
   ↓
   send/write can make progress


3. Partial Write
   ↓
   send() may send less than requested


4. Pending Output Buffer
   ↓
   keep unsent bytes
   and retry when POLLOUT occurs
```

And here we're getting very close to a **real event-driven server**.

## References

* [Linux `poll(2)`: POLLIN/POLLOUT and revents](https://man7.org/linux/man-pages/man2/poll.2.html)
* [Linux `send(2)`: return value and nonblocking behavior](https://man7.org/linux/man-pages/man2/send.2.html)
* [Linux `socket(7)`: socket buffer concepts](https://man7.org/linux/man-pages/man7/socket.7.html)

## What's Next

Now that `POLLOUT` is solid, the next step is a **TCP Echo Server with `poll()` that actually supports partial writes**: a `ClientConnection` for each client, a send buffer, and enabling/disabling `POLLOUT` as needed. After that, we'll be ready to move to `epoll` and understand why it's better than `poll` on Linux.
