# `epoll` with `stdin`

Okay. We'll follow the same **mental model** we built with `select()` and `poll()`, and we won't jump straight to TCP.

The order will be:

```text
1. epoll with stdin
        ↓
2. Understand epoll_create1()
        ↓
3. Understand epoll_ctl(ADD)
        ↓
4. Understand epoll_wait()
        ↓
5. Understand struct epoll_event
        ↓
6. TCP socket
        ↓
7. Multiple clients
        ↓
8. EPOLLIN + recv()
        ↓
9. Level-Triggered
        ↓
10. Edge-Triggered
```

## 1. First Experiment: `epoll` with `stdin`

The idea here is very simple.

Instead of saying:

> "I'm waiting on a socket"

we'll say:

> "I want Linux to notify me when **any file descriptor** I'm interested in becomes ready."

At the start we'll use:

```text
stdin = file descriptor 0
```

Meaning:

```text
Terminal
   │
   │ keyboard input
   ▼
stdin (fd = 0)
   │
   │
   ▼
   epoll
   │
   │ EPOLLIN
   ▼
our program
```

---

## 2. Creating the epoll Instance

The first thing:

```cpp
int epollFd = epoll_create1(0);
```

There's an important concept here.

### `epoll` is not a single function

With `poll()` we did:

```cpp
struct pollfd fds[10];
```

and then every time:

```cpp
poll(fds, 10, -1);
```

But `epoll` is a bit different.

We create an object inside the kernel that represents an **epoll instance**:

```text
Our Process
│
├── stdin       fd 0
├── socket      fd 3
├── socket      fd 4
│
└── epollFd     fd 5
                  │
                  ▼
              Kernel
              epoll instance
```

And `epollFd` itself is a **file descriptor**.

So you might see, for example:

```text
epollFd = 5
```

where `5` represents the epoll instance inside the kernel.

`epoll_create1()` creates a new epoll instance and returns a file descriptor referring to it. [epoll_create1(2) — Linux man page](https://man7.org/linux/man-pages/man2/epoll_create1.2.html)

---

## 3. Adding `stdin` to epoll

Now we have:

```cpp
int epollFd = epoll_create1(0);
```

But `epoll` is **not monitoring anything yet**.

We have to tell it:

> Watch `stdin`, and notify me when it's ready for reading.

We use:

```cpp
epoll_ctl()
```

The general form:

```cpp
epoll_ctl(
    epollFd,
    operation,
    fileDescriptor,
    &event
);
```

And we write:

```cpp
struct epoll_event event{};

event.events = EPOLLIN;
event.data.fd = STDIN_FILENO;

epoll_ctl(
    epollFd,
    EPOLL_CTL_ADD,
    STDIN_FILENO,
    &event
);
```

Let's break it down one piece at a time.

### `EPOLL_CTL_ADD`

It means:

```text
Add this FD to my epoll interest list
```

Meaning:

```text
epoll
 │
 ├── stdin
 │
 └── ...
```

You're telling the kernel:

> Add `stdin` to the file descriptors whose state I'm interested in.

---

## 4. What does `event.events = EPOLLIN` mean?

This one is important.

```cpp
event.events = EPOLLIN;
```

It means:

> I'm interested in knowing when this FD becomes ready for reading.

For `stdin`:

```text
Keyboard
   │
   │ user types
   ▼
stdin
   │
   │ data available
   ▼
EPOLLIN
```

So `EPOLLIN` here is very similar to the idea of:

```cpp
POLLIN
```

that we learned with `poll()`.

Roughly:

```text
poll()                     epoll

POLLIN          ────────►  EPOLLIN

POLLOUT         ────────►  EPOLLOUT
```

But the API architecture is different.

---

## 5. `event.data.fd`

The line:

```cpp
event.data.fd = STDIN_FILENO;
```

is very important.

Why?

Because when we use:

```cpp
epoll_wait()
```

the kernel returns the events that happened.

And when an event comes back, we need to know:

> Which FD did this event happen on?

So we store it along with the event:

```text
event
┌──────────────────────┐
│ events = EPOLLIN     │
│ data.fd = 0          │
└──────────────────────┘
```

So when this comes back:

```cpp
events[i]
```

we can do:

```cpp
events[i].data.fd
```

and learn:

```text
fd = 0
```

which is `stdin`.

---

## 6. Now `epoll_wait()`

After adding:

```text
stdin
```

to epoll, we need to say:

> Wait until some event happens.

We use:

```cpp
epoll_wait()
```

The form:

```cpp
int count = epoll_wait(
    epollFd,
    events,
    MAX_EVENTS,
    -1
);
```

Notice there's an `events` here.

For example:

```cpp
constexpr int MAX_EVENTS = 10;

struct epoll_event events[MAX_EVENTS];
```

This is **not the same** as the `event` we used in `epoll_ctl()`.

We have two separate concepts:

### Event for the interest list

```cpp
struct epoll_event event{};
```

We use it with:

```cpp
epoll_ctl()
```

to tell the kernel:

> What am I interested in?

---

### Events for the ready list

```cpp
struct epoll_event events[MAX_EVENTS];
```

We use it with:

```cpp
epoll_wait()
```

so the kernel can tell us:

> What became ready?

And this is a very important difference.

---

## 7. The Full Picture

Now we have:

```text
                    USER SPACE
┌──────────────────────────────────────┐
│                                      │
│  event                               │
│  ┌──────────────────────────────┐    │
│  │ events = EPOLLIN             │    │
│  │ data.fd = STDIN_FILENO       │    │
│  └──────────────┬───────────────┘    │
│                 │                    │
│                 │ epoll_ctl(ADD)     │
│                 ▼                    │
│           ┌────────────┐             │
│           │  epollFd   │             │
│           └─────┬──────┘             │
│                 │                    │
│                 │ epoll_wait()       │
│                 ▼                    │
│       struct epoll_event events[]    │
│                                      │
└──────────────────────────────────────┘
                  │
                  ▼
               KERNEL
```

---

## 8. See the Code: [stdin epoll code](./main.cpp)

## 9. The Flow While Running

When the program starts:

```text
epoll_create1()
      │
      ▼
epollFd
      │
      ▼
epoll_ctl(ADD, stdin, EPOLLIN)
      │
      ▼
epoll_wait()
      │
      │
      │  BLOCK
      │
      ▼
```

The program sits here:

```cpp
epoll_wait(...);
```

and doesn't do any manual polling.

Meaning it does **not** do this:

```cpp
while (true)
{
    check stdin;
    check socket;
    check socket;
    ...
}
```

Instead:

```text
              Kernel
                 │
                 │ wait
                 ▼
           ┌───────────┐
           │  epoll    │
           └─────┬─────┘
                 │
                 │ stdin becomes readable
                 ▼
             wake up
                 │
                 ▼
           epoll_wait()
             returns
                 │
                 ▼
             EPOLLIN
```

---

## 10. What Happens When You Type in the Terminal?

Say you typed:

```text
hello
```

The terminal puts the data into `stdin`'s input buffer.

The state roughly becomes:

```text
stdin
┌──────────────────────┐
│ h e l l o \n         │
└──────────────────────┘
          │
          │ readable
          ▼
       EPOLLIN
```

The kernel knows that:

```text
stdin is readable
```

So:

```cpp
epoll_wait()
```

wakes up and returns:

```cpp
count = 1;
```

and:

```cpp
events[0]
```

will contain something like:

```text
events[0]
┌──────────────────────────┐
│ events = EPOLLIN         │
│ data.fd = 0              │
└──────────────────────────┘
```

So we do:

```cpp
int fd = events[0].data.fd;
```

and get:

```text
fd = 0
```

So we know:

```cpp
if (fd == STDIN_FILENO)
```

and then:

```cpp
read(fd, buffer, ...);
```

---

## 11. The Most Important Mental Model So Far

Memorize this sentence:

> **`epoll_ctl()` defines what I'm interested in, and `epoll_wait()` tells me what became ready.**

Meaning:

```text
                epoll_ctl()
                    │
                    ▼
             WHAT DO I WANT?
             ┌──────────────┐
             │ stdin        │
             │ EPOLLIN      │
             └──────────────┘


                epoll_wait()
                    │
                    ▼
             WHAT IS READY?
             ┌──────────────┐
             │ stdin        │
             │ EPOLLIN      │
             └──────────────┘
```

And this differs from `poll()` in shape.

With `poll()` we did:

```cpp
pollfd fds[];
poll(fds, ...);
```

With `epoll`, the model is closer to:

```text
               epoll instance
                     │
        ┌────────────┼────────────┐
        ▼            ▼            ▼
      stdin        socket        socket
      EPOLLIN      EPOLLIN       EPOLLIN
```

and the kernel keeps the interest information instead of us resending a whole array every time.

---

## 12. And Then TCP

Once we understand this experiment, replacing:

```text
stdin
```

with:

```text
listening socket
```

becomes very natural:

```text
                    epoll
                      │
                      │
             EPOLLIN on server socket
                      │
                      ▼
                   accept()
                      │
          ┌───────────┼───────────┐
          ▼           ▼           ▼
       client 1    client 2    client 3
          │           │           │
       EPOLLIN      EPOLLIN      EPOLLIN
          │           │           │
        recv()      recv()      recv()
```

And here we'll clearly see why `epoll` is such a good fit for **multiple clients**.

After building the TCP server, we'll stop at a very important point: **why can `epoll_wait()` return the same FD again even when no new data arrived?** From there we'll move on to the difference between **Level-Triggered** and **Edge-Triggered**, and then `EPOLLET` and why you must read until `EAGAIN`.
