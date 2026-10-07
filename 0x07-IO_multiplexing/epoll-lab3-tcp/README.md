# Building a TCP Server with `epoll`

Okay. Now we move from **`stdin` to a TCP socket**, and we'll build our first TCP server with `epoll` step by step, rather than writing all the code at once.

## 1. The New Mental Model

With `stdin` we had:

```text
stdin
  │
  │ EPOLLIN
  ▼
epoll_wait()
  │
  ▼
read()
```

Now we'll replace `stdin` with a **listening TCP socket**:

```text
                    epoll
                      │
                      │ EPOLLIN
                      ▼
              listening socket
                      │
                      │
                  accept()
                      │
          ┌───────────┼───────────┐
          ▼           ▼           ▼
       client 1    client 2    client 3
```

But there's a very important point:

> `EPOLLIN` on the listening socket **does not mean a client sent data**.

For a listening socket, it means:

> **There is a pending connection, and `accept()` can succeed without blocking.**

This is the first important difference we need to lock in.

---

## 2. Starting with the Listening Socket

As we did before:

```cpp
int serverFd = socket(
    AF_INET,
    SOCK_STREAM,
    0
);
```

Then:

```text
socket()
   │
   ▼
serverFd
   │
   ▼
bind()
   │
   ▼
listen()
```

After `listen()`, the socket becomes:

```text
             TCP Server
                 │
                 ▼
          listening socket
                 │
                 │ waiting for
                 │ connections
                 ▼
             port 12345
```

---

## 3. It Must Be Non-Blocking

Here's a very important point.

With `epoll`, we'll make the sockets:

```text
NON-BLOCKING
```

Meaning:

```cpp
fcntl(
    serverFd,
    F_SETFL,
    O_NONBLOCK
);
```

Why?

Because we don't want this to happen:

```cpp
accept()
   │
   ▼
BLOCK
```

inside the event loop.

The event loop must stay able to serve all the clients.

---

## 4. Adding the Listening Socket to epoll

After:

```cpp
listen(serverFd, SOMAXCONN);
```

we do:

```cpp
struct epoll_event event{};

event.events = EPOLLIN;
event.data.fd = serverFd;

epoll_ctl(
    epollFd,
    EPOLL_CTL_ADD,
    serverFd,
    &event
);
```

The kernel now knows:

```text
epoll
 │
 └── serverFd
       │
       └── interested in EPOLLIN
```

---

## 5. What Does EPOLLIN Mean Here?

This is the most important point at this stage.

We have:

```text
serverFd
```

which is a listening socket.

When a client calls:

```text
connect()
```

a TCP handshake takes place:

```text
Client                         Server

   SYN ───────────────────────►

       ◄──────────────── SYN-ACK

   ACK ───────────────────────►

                                  │
                                  ▼
                          connection pending
                                  │
                                  ▼
                              EPOLLIN
```

So:

```cpp
epoll_wait()
```

wakes up and returns:

```text
serverFd
EPOLLIN
```

This means:

```text
There is a connection ready to be accepted.
```

and not:

```text
There is application data ready to recv().
```

---

## 6. `accept()` Now

When we find:

```cpp
if (fd == serverFd)
```

we do:

```cpp
int clientFd = accept(
    serverFd,
    nullptr,
    nullptr
);
```

Now we have:

```text
serverFd
    │
    │ accept()
    ▼
clientFd
```

It's very important to separate the two:

```text
serverFd
   │
   └── listening socket
       its job is accept()


clientFd
   │
   └── connected socket
       its job is recv()/send()
```

---

## 7. What Do We Do After `accept()`?

Here comes the nice idea in `epoll`.

We now have:

```text
clientFd = 4
```

So we add it to the same `epoll`:

```cpp
struct epoll_event clientEvent{};

clientEvent.events = EPOLLIN;
clientEvent.data.fd = clientFd;

epoll_ctl(
    epollFd,
    EPOLL_CTL_ADD,
    clientFd,
    &clientEvent
);
```

The picture becomes:

```text
                    epoll
                      │
          ┌───────────┴───────────┐
          │                       │
          ▼                       ▼
      serverFd                 clientFd
      EPOLLIN                  EPOLLIN
          │                       │
       accept()                 recv()
```

And if another client connects:

```text
                    epoll
                      │
          ┌───────────┼───────────┐
          ▼           ▼           ▼
      serverFd     clientFd1   clientFd2
      EPOLLIN      EPOLLIN     EPOLLIN
```

And here we start to see the power of `epoll`.

---

## 8. The First Complete TCP Server

Let's make a very simple version:

Code in [main.cpp](./main.cpp)

---

## 9. Looking at the Event Loop Up Close

The most important part of the whole program is:

```cpp
while (true)
{
    int count = epoll_wait(...);

    for (int i = 0; i < count; ++i)
    {
        int fd = events[i].data.fd;

        if (fd == serverFd)
        {
            accept(...);
        }
        else
        {
            recv(...);
        }
    }
}
```

This is roughly the beginning of **event-driven architecture**.

```text
                 ┌───────────────┐
                 │ epoll_wait()  │
                 └───────┬───────┘
                         │
                         ▼
                  Something ready
                         │
              ┌──────────┴──────────┐
              │                     │
              ▼                     ▼
        serverFd ready          clientFd ready
              │                     │
              ▼                     ▼
          accept()               recv()
              │                     │
              ▼                     ▼
        new client              data
              │
              ▼
       epoll_ctl(ADD)
```

---

## 10. Testing Multiple Clients

Run the server:

```bash
./server
```

Then open another terminal:

```bash
nc 127.0.0.1 12345
```

Then a third terminal:

```bash
nc 127.0.0.1 12345
```

You might see:

```text
Server listening on port 12345

New client: fd=5
New client: fd=6
```

Then type in the first client:

```text
hello from client 1
```

In the event loop, this happens:

```text
epoll_wait()
    │
    ▼
fd = 5
EPOLLIN
    │
    ▼
recv(5)
```

And if client 2 sends:

```text
hello from client 2
```

it might return:

```text
epoll_wait()
    │
    ▼
fd = 6
EPOLLIN
    │
    ▼
recv(6)
```

**The same `epoll_wait()` serves both.**

And this is exactly the essence of:

```text
I/O Multiplexing
```

---

## 11. But We Have a Very Important Problem

Notice that we wrote:

```cpp
int clientFd = accept(...);
```

only once.

That's enough at the beginning to understand the API, but with `epoll` there's a very important point:

If these arrive:

```text
connection 1
connection 2
connection 3
```

before we get back to `epoll_wait()`, the listening socket may still have more than one pending connection.

That's why, in a bit, we'll change:

```cpp
accept()
```

to:

```cpp
while (true)
{
    accept();

    if (errno == EAGAIN)
        break;
}
```

And this leads us straight to a very important concept:

> **Why do we need to drain until `EAGAIN`?**

From here we start **Level-Triggered vs Edge-Triggered**.

This isn't a side detail; it's the foundation for understanding `EPOLLET`.

The core semantics of `epoll`, `epoll_ctl`, and `epoll_wait` are documented in the official Linux man-pages: [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html), [epoll_ctl(2)](https://man7.org/linux/man-pages/man2/epoll_ctl.2.html), [epoll_wait(2)](https://man7.org/linux/man-pages/man2/epoll_wait.2.html).

## What's Next

The logical next step is to pause here and understand **Level-Triggered** through this same TCP server, then add an `accept()` loop until `EAGAIN`, and after that enable `EPOLLET` and see for ourselves what changes.
