# Explain loop in server.cpp

```cpp
while (connected) {
    int n = client.receive(buffer, sizeof(buffer));
    if (n == 0) {
        std::cout << "Client disconnected." << std::endl;
        break;
    }
    pending.append(buffer, n);

    // Process every complete line currently in `pending`
    size_t pos;
    while ((pos = pending.find('\n')) != std::string::npos) {
        std::string line = pending.substr(0, pos);
        pending.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();            // telnet sends \r\n
        }

        if (line == "close") {
            std::cout << "Close command received. Shutting down." << std::endl;
            std::string bye = "Server closing.\n";
            client.send(bye.data(), bye.size());
            running = false;            // stops the accept loop
            connected = false;          // stops the client loop
            break;
        }

        std::cout << "Received: " << line << std::endl;
    }
}
```

This loop turns TCP's raw byte stream into complete lines, so it can handle each command separately. The key fact is that TCP has no concept of messages. If a client sends `"hello\n"`, the server's `receive` might return `"hel"` and then `"lo\n"`, or it might return `"hello\nclose\n"` in one go. The loop deals with that.

## The outer loop: `while (connected)`

```cpp
int n = client.receive(buffer, sizeof(buffer));
```
This blocks until the client sends something, then fills `buffer` with up to 1024 bytes and returns how many bytes arrived. Because it's a stream, `n` has no relationship to how many messages the client sent.

```cpp
if (n == 0) {
    std::cout << "Client disconnected." << std::endl;
    break;
}
```
A return value of 0 from `recv` means the client closed the connection cleanly. `break` leaves this client's loop, and the outer accept loop then waits for the next client. (Errors never reach this check, because your `receive` throws on `-1`, which lands in the `catch`.)

```cpp
pending.append(buffer, n);
```
The `buffer` is overwritten on every `receive`, so you can't rely on it to remember earlier data. `pending` is the accumulator that persists across iterations. Only the first `n` bytes of `buffer` are valid, which is why `append` takes both the pointer and the length. Using `std::string` rather than a C string also means embedded `\0` bytes won't cut the data off.

## The inner loop: extracting complete lines

```cpp
while ((pos = pending.find('\n')) != std::string::npos) {
```
`find('\n')` returns the index of the first newline in `pending`, or `std::string::npos` if there is none. The assignment happens inside the condition, so `pos` is set and tested in one step. The loop runs as long as `pending` contains at least one complete line, and stops when only a partial line (or nothing) is left.

```cpp
std::string line = pending.substr(0, pos);
pending.erase(0, pos + 1);
```
`substr(0, pos)` copies everything before the newline, so the `\n` itself is not included. `erase(0, pos + 1)` then removes that line *and* its newline from the front of `pending`. After this, `pending` holds only the data that hasn't been processed yet.

```cpp
if (!line.empty() && line.back() == '\r') {
    line.pop_back();
}
```
Telnet (and some other clients) end lines with `\r\n`. After splitting on `\n`, a stray `\r` would remain at the end, and `"close\r" == "close"` would be false. The `!line.empty()` check avoids calling `back()` on an empty string, which is undefined behavior.

```cpp
if (line == "close") {
    ...
    client.send(bye.data(), bye.size());
    running = false;
    connected = false;
    break;
}
```
When the command matches, the server sends a goodbye message, then sets two flags. `running = false` makes the outer accept loop stop after this client is finished, and `connected = false` makes the client loop stop. The `break` only exits the *inner* `while`, which is why the flags are needed: control returns to `while (connected)`, which sees `false` and exits, and then `while (running)` does the same.

```cpp
std::cout << "Received: " << line << std::endl;
```
Any line that isn't `close` is treated as an ordinary message.

## A worked example

Suppose the client sends `hello\nclose\n`, but it arrives in three pieces:

| `receive` returns | `pending` after append | What happens |
|---|---|---|
| `"hel"` | `"hel"` | No `\n`, so the inner loop doesn't run. Back to `receive`. |
| `"lo\nclo"` | `"hello\nclo"` | `\n` found at index 5. `line = "hello"`, `pending = "clo"`. Prints `Received: hello`. No more `\n`, so the inner loop ends. |
| `"se\n"` | `"close\n"` | `\n` at index 5. `line = "close"`, `pending = ""`. The command matches, so the server shuts down. |

The opposite case works too. If `"a\nb\nc\n"` arrives in a single `receive`, the inner loop runs three times, once per line, before the server calls `receive` again.

## Caveats

- **Unbounded growth:** if a client never sends a `\n`, `pending` grows forever. A real server would cap its size and disconnect clients that exceed it.
- **Data after `close` is discarded:** if the client sends `close\nmore\n` in one packet, `more` is dropped because the loops exit. For a shutdown command that's fine.
- **Partial last line on disconnect:** if the client disconnects mid-line (say it sent `"clos"` and then closed), the leftover `"clos"` in `pending` is silently thrown away when the loop breaks.