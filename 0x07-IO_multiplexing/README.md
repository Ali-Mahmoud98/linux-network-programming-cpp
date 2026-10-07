# `epoll()` System Call

This is important because `epoll` is the primary mechanism you will encounter on Linux when building **high-concurrency servers**; it is also essential for understanding what happens under the hood of certain **asynchronous networking frameworks**.


### Why do we need `epoll`?

First question: why do we need `epoll`?

Imagine we have:

```text
10,000 connections

fd 4
fd 5
fd 6
...
fd 10003
```

With `poll()`, on every iteration you pass in a large array:

> مع `poll()` عند كل iteration أنت تمرر `array` كبيرة

* `poll()`:
    * fd 4
    * fd 5
    * fd 6
    * ...
    * fd 10003

Then you have to scan the results to find out which sockets became ready.

`epoll` gives us a different model:

```mermaid
flowchart LR
    E["epoll instance"] --> R["register"]
    E --> W["wait"]
    R --> M["monitored fds"]
    W --> RE["ready events"]
```

Instead of thinking:
> "Every time, check all the fds."

We want to tell the kernel:
> "I'm interested in these sockets. When one of them becomes ready, give me the ready events."


### The 3 Core APIs

We'll start by focusing on just three functions.

#### 1. `epoll_create1()`

Creates an epoll instance:

```cpp
int epfd = epoll_create1(0);
```

Think of it like this:

```mermaid
flowchart TD
    epfd --> EI
    subgraph EI["epoll instance"]
        MS["monitored sockets"]
    end
```

#### 2. `epoll_ctl()`

You tell the kernel: "I want to monitor this fd."

For example:

```cpp
epoll_ctl(
    epfd,
    EPOLL_CTL_ADD,
    server_fd,
    &event
);
```

The operations are:

```c++
EPOLL_CTL_ADD
EPOLL_CTL_MOD
EPOLL_CTL_DEL
```

Which means:
* ADD → add an fd
* MOD → change the events being monitored for it
* DEL → remove an fd

#### 3. `epoll_wait()`

This is the most important one:

```cpp
int n = epoll_wait(
    epfd,
    events,
    MAX_EVENTS,
    -1
);
```

It means:
> Wait until some file descriptors become ready.

For example:

```mermaid
flowchart LR
    A["epoll_wait()"] -->|blocking| B["Kernel"]
    B --> C["client 5 becomes readable"]
    C --> D["return"]
    D --> E["ready event"]
```

### The New Mental Model

With `poll()`:

```mermaid
flowchart LR
    A["poll()"] --> B
    subgraph B["pollfd array"]
        direction TB
        F3["fd 3"] ~~~ F4["fd 4"] ~~~ F5["fd 5"] ~~~ F6["fd 6"] ~~~ Dots["..."]
    end
```

With `epoll`:

```mermaid
flowchart TD
    EI["epoll instance"] --> FD3["fd3"]
    EI --> FD4["fd4"]
    EI --> FD5["fd5"]

    FD3 --> EW["epoll_wait()"]
    FD4 --> EW
    FD5 --> EW

    EW --> RE["Ready events"]
```

And this is the very important conceptual difference:

```mermaid
flowchart LR
    subgraph Poll["poll"]
        direction TB
        P1["poll()"] --> P2["Monitor this set of fds"]
    end

    subgraph Epoll["epoll"]
        direction TB
        E1["epoll"] --> E2["Keep the set of fds I'm monitoring,<br/>and tell me which fds became ready"]
    end

    Poll ~~~ Epoll
```

### Check the LAB1: [LAB1 Directory](./epoll-lab1/)

