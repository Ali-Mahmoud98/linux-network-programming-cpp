# THE ISSUE with debug logs

Terminal1:
```bash
❯ ./build/asio-tcp-server
[DEBUG] ./server/Server.cpp:17 Server created, this= 0x7fffd932ddb0
[DEBUG] ./main.cpp:10 Main: Server created
[DEBUG] ./server/Server.cpp:22 Server starting, this= 0x7fffd932ddb0
[DEBUG] ./connection/Connection.cpp:10 Connection created, this= 0x64f2b1e06600
[DEBUG] ./server/Server.cpp:29 Server->accept method started, this= 0x64f2b1e06600 use_count= 1
[DEBUG] ./server/Server.cpp:49 Server->accept method ended, this= 0x64f2b1e06600 use_count= 2
[DEBUG] ./main.cpp:14 Main: IO context running
[DEBUG] ./server/Server.cpp:35 Server->accept callback started, this= 0x64f2b1e06600 use_count= 1
Client connected!
[DEBUG] ./connection/Connection.cpp:20 Connection starting, this= 0x64f2b1e06600
[DEBUG] ./connection/Connection.cpp:26 Connection->read method started, this= 0x64f2b1e06600
[DEBUG] ./connection/Connection.cpp:47 Connection->read method ended, this= 0x64f2b1e06600
[DEBUG] ./connection/Connection.cpp:10 Connection created, this= 0x64f2b1e06be0
[DEBUG] ./server/Server.cpp:29 Server->accept method started, this= 0x64f2b1e06be0 use_count= 1
[DEBUG] ./server/Server.cpp:49 Server->accept method ended, this= 0x64f2b1e06be0 use_count= 2
[DEBUG] ./server/Server.cpp:46 Server->accept callback ended, this= 0x64f2b1e06600 use_count= 1
Read error: Operation aborted.
[DEBUG] ./server/Server.cpp:35 Server->accept callback started, this= 0x64f2b1e06be0 use_count= 1
Client connected!
[DEBUG] ./connection/Connection.cpp:20 Connection starting, this= 0x64f2b1e06be0
[DEBUG] ./connection/Connection.cpp:26 Connection->read method started, this= 0x64f2b1e06be0
[DEBUG] ./connection/Connection.cpp:47 Connection->read method ended, this= 0x64f2b1e06be0
[DEBUG] ./connection/Connection.cpp:10 Connection created, this= 0x64f2b1e07110
[DEBUG] ./server/Server.cpp:29 Server->accept method started, this= 0x64f2b1e07110 use_count= 1
[DEBUG] ./server/Server.cpp:49 Server->accept method ended, this= 0x64f2b1e07110 use_count= 2
[DEBUG] ./server/Server.cpp:46 Server->accept callback ended, this= 0x64f2b1e06be0 use_count= 1
Read error: Operation aborted.

```

Terminal2:
```bash

❯ nc localhost 8080
Hello
❯ nc localhost 8080
Hello
❯  
```

The output shows a lifetime bug: each `Connection` is destroyed right after it's accepted, so its pending read is cancelled and the echo never happens.

## What the log shows, step by step

**Startup**
```
Server created → Server starting
Connection created, this=...6600
accept method started  use_count= 1
accept method ended    use_count= 2
IO context running
```
`accept()` creates a `Connection` inside a `shared_ptr`. While `accept()` is still running, two things own it: the local `connection` variable and the copy captured by the accept lambda, hence `use_count=2`. Then `accept()` returns and `io.run()` starts waiting for a client.

**First client connects (`nc localhost 8080`)**
```
accept callback started  use_count= 1
Client connected!
Connection starting → read method started → read method ended
Connection created (...6be0) → accept started → accept ended   (the next accept)
accept callback ended    use_count= 1
Read error: Operation aborted.
```
- `use_count` is now 1 because the local variable from `accept()` is long gone. The only owner is the accept lambda's captured copy.
- `connection->start()` calls `read()`, which posts `async_read_some` with a lambda that captures only the raw `this`. It doesn't hold a `shared_ptr`.
- The callback then calls `accept()` for the next client and returns.
- When the callback returns, Asio destroys the accept lambda and its captured `shared_ptr`. The count goes 1 → 0, so the `Connection` and its socket are destroyed.
- Destroying the socket cancels the pending `async_read_some`. Its handler is queued with `operation_aborted`, which prints `Read error: Operation aborted.`

You never see `Received N bytes` or `Sent N bytes` because the connection is gone before the client's data can be read. `nc` just sees the connection die. The second `nc` run repeats the same sequence with `...6be0`.

## Why it doesn't crash

The read lambda holds a dangling `this`, but the error path only prints `error.message()` and returns without touching any member. That's luck. If the lambda ever used `this` (for example `m_buffer` or `write(...)`) after destruction, you'd have a use-after-free.

## The fix

NOTE: the fix will be in the next lesson.

The pending async operation must keep the connection alive. Use `enable_shared_from_this` and capture `shared_from_this()` in the handlers.

```cpp
// Connection.hpp
class Connection : public std::enable_shared_from_this<Connection>
{ /* ... */ };
```

```cpp
void Connection::read()
{
    auto self = shared_from_this();
    m_socket.async_read_some(
        asio::buffer(m_buffer),
        [this, self](const asio::error_code& error, std::size_t bytes_read)
        {
            if (error) { /* log */ return; }
            write(bytes_read);
        });
}

void Connection::write(std::size_t bytes_read)
{
    auto self = shared_from_this();
    asio::async_write(
        m_socket,
        asio::buffer(m_buffer.data(), bytes_read),
        [this, self](const asio::error_code& error, std::size_t)
        {
            if (error) { /* log */ return; }
            read();
        });
}
```

Now the lifecycle works like this:
- Each pending async operation holds a `shared_ptr`, so the connection stays alive while it is waiting on I/O.
- When the client disconnects, the read completes with `eof`, the handler returns without starting another operation, and the last `shared_ptr` drops. The connection is destroyed cleanly.

`shared_from_this()` only works if the object is already owned by a `shared_ptr`. That's true here because you create it with `make_shared`.

## A debugging tip

Add a destructor log to `Connection`:
```cpp
Connection::~Connection() { DEBUG_PRINT("Connection destroyed, this= " << this); }
```
In your current output it would have appeared right after `accept callback ended`, which would have shown the problem immediately.
