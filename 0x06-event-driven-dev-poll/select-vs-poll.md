# select() vs poll() (and epoll)

When we're talking about a **modern Linux TCP server**, **`poll()` is better than `select()`**, but not because `poll()` is magically faster. The main differences are in the API and its limitations.

## Quick comparison

|                                | `select()`   | `poll()`                              |
| ------------------------------ | ------------ | ------------------------------------- |
| API                            | Older        | Newer than `select`                   |
| How fds are represented        | `fd_set`     | `pollfd[]`                            |
| `FD_SETSIZE` limit             | **Yes**      | No such limit in `poll()` itself      |
| Checking events                | `FD_ISSET()` | `revents`                             |
| Adding/removing an fd          | Less flexible | Easier                               |
| Code readability               | Average      | Better                                |
| Good for learning              | ⭐⭐⭐⭐⭐        | ⭐⭐⭐⭐⭐                                 |
| Good for a large Linux server  | ❌            | ⚠️ Limited compared to `epoll`        |

The core idea:

```mermaid
flowchart LR
    subgraph Select["select()"]
        direction TB
        S1["select()"] --> S2["fd_set"]
        S2 --> S3["limited by FD_SETSIZE"]
    end

    subgraph Poll["poll()"]
        direction TB
        P1["poll()"] --> P2["pollfd[]"]
        P2 --> P3["does not depend on FD_SETSIZE"]
    end

    subgraph Epoll["epoll()"]
        direction TB
        E1["epoll()"] --> E2["kernel-managed event mechanism"]
        E2 --> E3["suited for a large number of connections"]
    end

    Select ~~~ Poll ~~~ Epoll
```

## So why did we study `select()` first?

Because `select()` is excellent for understanding **I/O multiplexing**.

You have:

```mermaid
flowchart TD
    A["Many sockets"] --> B["Event Waiter"]
    B --> C["Who's ready?"]
    C --> D["socket 4<br/>READ"]
    C --> E["socket 7<br/>READ"]
```

This is the mental model we'll need later with:

```mermaid
flowchart LR
    A["select"] --> B["poll"]
    B --> C["epoll"]
    C --> D["Asio"]
```

## In a real project, which one do I pick?

If you're on Linux:

```mermaid
flowchart TD
    A["Few connections"] -->|possible| B["select()"]
    A -->|better| C["poll()"]

    D["Many connections"] -->|the natural choice on Linux| E["epoll()"]
```

For the **C++ / Linux / System Programming** path we're following, I don't recommend skipping `poll()` and jumping straight to `epoll()`.

I want you to see the difference yourself:

```mermaid
flowchart LR
    subgraph Select["select"]
        direction TB
        S1["fd_set"] --> S2["select()"] --> S3["FD_ISSET()"]
    end

    subgraph Poll["poll"]
        direction TB
        P1["pollfd[]"] --> P2["poll()"] --> P3["revents"]
    end

    subgraph Epoll["epoll"]
        direction TB
        E1["epoll instance"] --> E2["epoll_wait()"] --> E3["ready events"]
    end

    Select ~~~ Poll ~~~ Epoll
```

## Summary

`select()` is the best entry point for learning, `poll()` is a better API than `select()`, and **`epoll()` is the most important one when talking about Linux servers with a large number of connections**.
