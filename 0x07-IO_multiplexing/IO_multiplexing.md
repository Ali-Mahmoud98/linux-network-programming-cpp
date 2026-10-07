# Small Review

## What is I/O Multiplexing?

In a nutshell:

**I/O Multiplexing** means managing and monitoring multiple I/O sources using a single mechanism.

In networking:

```mermaid
flowchart TD
    M["I/O Multiplexing"] --> FD3["fd 3<br/>socket"]
    M --> FD4["fd 4<br/>socket"]
    M --> FD5["fd 5<br/>socket"]

    FD3 --> SEL["select / poll / epoll"]
    FD4 --> SEL
    FD5 --> SEL

    SEL --> Q["Which one is ready for I/O?"]
```

### What does the word "multiplexing" itself mean?

Multiplex = multiple + ... meaning to combine/manage several channels or sources over a shared resource or path.

In our case:

Instead of creating a thread or a blocking call for each socket, you use a single mechanism to monitor multiple sockets.

Therefore:

```mermaid
flowchart LR
    A["I/O Multiplexing"] --> B["Multiple I/O"]
    B --> C["One monitoring mechanism"]
```

This is exactly why `select()`, `poll()`, and `epoll()` are called I/O multiplexing mechanisms.
