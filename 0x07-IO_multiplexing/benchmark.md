# Benchmarking the Servers

Goal: compare `select()`, `poll()`, and `epoll()` servers with the same load, and pick the better one using numbers.

## Servers under test

| Name | Source | Notes |
|---|---|---|
| select | `../0x06-event-driven-dev-poll/select-tcp-server/main.cpp` | Echo. Max `FD_SETSIZE` (1024) fds. |
| poll | `../0x06-event-driven-dev-poll/poll-tcp-server/main.cpp` | Echo. |
| poll-partial | `poll-echo-server-partial-writes/main.cpp` | Echo with an output buffer and `POLLOUT`. |
| epoll | `epoll-lab3-tcp/main.cpp` | **Does not echo yet.** Needs a small patch (Step 1). |

All of them listen on port `12345`, so run **one server at a time**.

## Step 0: Make the test fair

Things in the current code that would give wrong results:

1. **Logging:** every server prints a line per `recv()`. Printing can cost more than the I/O itself. Run each server with `> /dev/null`.
2. **Debug build:** build with optimizations (`-O2`), not the default CMake build.
3. **Client limit:** select, poll, and poll-partial have `MAX_CLIENTS = 10`. More clients get `Too many clients`. For the big tests, change it to `1000` in each of the three files (do not go above ~1000 for select, because of `FD_SETSIZE`).
4. **Listen backlog:** `listen(fd, 10)` drops connections when many clients connect at once. Change `10` to `SOMAXCONN` in select, poll, and poll-partial.
5. **File descriptor limit:** run `ulimit -n 65535` in the terminal you use for both server and client.

## Step 1: Make epoll echo

In `epoll-lab3-tcp/main.cpp`, right after the `std::cout << "Client " ...` block (inside `if (bytesRead > 0)`), add:

```c++
send(fd, buffer, bytesRead, 0);
```

Without it, the client waits forever for a reply.

Also, `epoll-lab3-tcp` is not in [CMakeLists.txt](CMakeLists.txt). Add this line there:

```cmake
add_subdirectory(epoll-lab3-tcp)
```

## Step 2: Build with optimizations

From the repo root:

```bash
cmake -S 0x06-event-driven-dev-poll -B build/0x06 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2"
cmake --build build/0x06

cmake -S 0x07-IO_multiplexing -B build/0x07 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2"
cmake --build build/0x07
```

The binaries are:

* `build/0x06/select-tcp-server/select_tcp_server_lab`
* `build/0x06/poll-tcp-server/poll_tcp_server_lab`
* `build/0x07/poll-echo-server-partial-writes/poll_echo_server`
* `build/0x07/epoll-lab3-tcp/epoll-lab3-tcp`

## Step 3: The load generator

Save this as `bench_echo.py` (next to this file). It opens many connections, then each connection does ping-pong: send a message, wait for the same message back, repeat.

```python
import asyncio
import statistics
import sys
import time

HOST, PORT = "127.0.0.1", 12345


async def connect():
    return await asyncio.open_connection(HOST, PORT)


async def worker(reader, writer, payload, stop_at, lats, errors):
    n = len(payload)
    try:
        while time.perf_counter() < stop_at:
            t0 = time.perf_counter()
            writer.write(payload)
            await reader.readexactly(n)
            lats.append(time.perf_counter() - t0)
    except (asyncio.IncompleteReadError, ConnectionError):
        errors.append(1)
    finally:
        writer.close()


async def main():
    active = int(sys.argv[1])          # clients sending traffic
    duration = float(sys.argv[2])      # seconds
    size = int(sys.argv[3])            # message size in bytes
    idle = int(sys.argv[4]) if len(sys.argv) > 4 else 0  # connected but silent

    payload = b"x" * size

    # Connect everything first, so connection setup is not measured.
    idle_conns = [await connect() for _ in range(idle)]
    conns = [await connect() for _ in range(active)]

    lats, errors = [], []
    stop_at = time.perf_counter() + duration
    start = time.perf_counter()

    await asyncio.gather(*(
        worker(r, w, payload, stop_at, lats, errors) for r, w in conns
    ))

    elapsed = time.perf_counter() - start

    for _, w in idle_conns:
        w.close()

    if not lats:
        print("no successful requests")
        return

    lats.sort()
    pct = lambda p: lats[min(len(lats) - 1, int(len(lats) * p))] * 1000

    print(f"active={active} idle={idle} size={size}B duration={elapsed:.1f}s")
    print(f"requests : {len(lats)}")
    print(f"req/s    : {len(lats) / elapsed:.0f}")
    print(f"latency  : avg={statistics.mean(lats) * 1000:.3f}ms "
          f"p50={pct(0.50):.3f}ms p95={pct(0.95):.3f}ms p99={pct(0.99):.3f}ms")
    print(f"errors   : {len(errors)}")


asyncio.run(main())
```

Usage:

```bash
python3 bench_echo.py <active_clients> <seconds> <message_bytes> [idle_clients]
```

## Step 4: Run one test

Use three terminals.

**Terminal 1: server** (pinned to core 0, output hidden):

```bash
ulimit -n 65535
taskset -c 0 ./build/0x07/epoll-lab3-tcp/epoll-lab3-tcp > /dev/null
```

**Terminal 2: client** (pinned to a different core, so they do not fight for CPU):

```bash
ulimit -n 65535
taskset -c 2 python3 bench_echo.py 100 10 64
```

**Terminal 3: server CPU and memory** (while the client runs):

```bash
PID=$(pgrep -n -f epoll-lab3-tcp)
ps -o pid,cputime,rss,nlwp -p $PID   # before
# ... wait for the client to finish ...
ps -o pid,cputime,rss,nlwp -p $PID   # after
```

`cputime(after) - cputime(before)` is the CPU time the server used. For the same number of requests, less CPU time means a more efficient server.

Stop the server with `Ctrl+C` and repeat with the next server.

## Step 5: The scenarios

Run every scenario against every server, and write down the results.

| # | Scenario | Command | What it shows |
|---|---|---|---|
| A | Few clients, small messages | `python3 bench_echo.py 10 10 64` | Baseline. All servers should be close. |
| B | Many active clients | `python3 bench_echo.py 500 10 64` | How throughput and latency change with load. |
| C | Large messages | `python3 bench_echo.py 50 10 16384` | Cost of copying and partial writes (poll-partial handles them; others may drop or miss data). |
| D | Many idle clients, few active | `python3 bench_echo.py 10 10 64 900` | **The main select/poll vs epoll difference.** |

Why scenario D matters: `select()` and `poll()` scan every fd on each call, so cost grows with the number of connections, even idle ones. `epoll_wait()` returns only the ready fds, so idle connections cost almost nothing.

> Scenarios B and D need the changes from Step 0 (items 3 and 4). Keep select at 1000 clients or less.

## Step 6: Record results

Copy this table and fill it in (run each test 3 times and use the median):

| Server | Scenario | req/s | p50 (ms) | p99 (ms) | Errors | Server CPU time (s) | RSS (KB) |
|---|---|---|---|---|---|---|---|
| select | A | | | | | | |
| poll | A | | | | | | |
| poll-partial | A | | | | | | |
| epoll | A | | | | | | |
| ... | | | | | | | |

## Step 7: Optional deeper checks

Count syscalls while under load (shows how many `select`/`poll`/`epoll_wait` calls each server needs):

```bash
sudo strace -c -f -p $(pgrep -n -f <server-name>)
# Ctrl+C after the client finishes to see the summary
```

CPU counters:

```bash
perf stat -e task-clock,context-switches -p $(pgrep -n -f <server-name>) -- sleep 10
```

## How to choose

* **Correctness first:** servers with errors or wrong echo data are out, no matter how fast.
* **Scenarios A/B:** if the numbers are close, the choice does not matter for small loads.
* **Scenario D and server CPU time:** this is where `epoll` should win clearly as idle connections grow. `select` also has the 1024 fd hard limit.
* **Scenario C:** prefer the server that handles partial writes (`poll-partial`). To compare it fairly with epoll, give epoll the same output buffering.
* Pick the simplest server that meets your target connection count and latency. If you need thousands of connections, pick `epoll`.

## Limits of this test

* The Python client is single-threaded. If it hits 100% CPU, the client is the bottleneck, not the server. Check with `top`. If needed, run two or three client processes on different cores and add up the req/s.
* Everything runs on localhost, so there is no network delay.
* The servers are single-threaded lab code (no accept loop draining, no writes buffering except poll-partial), so results compare the I/O mechanism, not production quality.
