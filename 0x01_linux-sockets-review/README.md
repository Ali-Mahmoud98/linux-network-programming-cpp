# Linux Sockets Review

A small C++17 TCP client/server built on a RAII `Socket` wrapper, plus a cheat sheet of Linux tools for inspecting sockets.

## Project layout

```
.
├── CMakeLists.txt
├── interface/ISocket.hpp     # socket interface
├── socket/Socket.{hpp,cpp}   # RAII wrapper (static library "socket")
├── server/main.cpp           # sequential TCP server on port 8080
└── client/main.cpp           # sends one message and disconnects
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

```text
-- Configuring done
-- Generating done
[ 33%] Built target socket
[ 66%] Built target server
[100%] Built target client
```

The binaries are `build/server` and `build/client`.

## Run

### Server

```bash
./build/server
```

```text
Listening on port 8080...
```

The server handles one client at a time and reads newline-terminated messages.

| Input line | Behavior |
|------------|----------|
| any text   | printed as `Received: <text>` |
| `close`    | replies `Server closing.` and shuts the server down |
| EOF (client disconnects) | prints `Client disconnected.` and waits for the next client |

### Client

```bash
./build/client "<message>"
```

The client connects to `127.0.0.1:8080`, sends the message plus `\n`, and exits. Closing the connection does **not** stop the server.

## Example session

Terminal 1: start the server.

```bash
./build/server
```

Terminal 2: send messages with the client and with `nc`.

```bash
./build/client "Hello"
./build/client "Second message"
printf 'hi from nc\n' | nc -N localhost 8080
```

Terminal 1 output (the server keeps running after every client disconnects):

```text
Listening on port 8080...
Client connected!
Received: Hello
Client disconnected.
Destructor called.
Client connected!
Received: Second message
Client disconnected.
Destructor called.
Client connected!
Received: hi from nc
Client disconnected.
Destructor called.
```

Stop the server by sending the `close` command:

```bash
printf 'close\n' | nc -N localhost 8080
```

```text
Server closing.
```

Server output:

```text
Client connected!
Close command received. Shutting down.
Destructor called.
Server stopped.
Destructor called.
```

### Error cases

Server not running:

```bash
./build/client "Hello"
```

```text
Error: connect() failed
```

(exit code `1`; `nc -zv localhost 8080` reports `Connection refused`, the kernel answering the SYN with an RST.)

Missing argument:

```bash
./build/client
```

```text
Usage: ./build/client <message>
```

---

# Notes: Linux socket tools

## 1. Inspect sockets: `ss`, `netstat`, `lsof`

```bash
ss -tln                 # TCP, Listening, Numeric ports
ss -tlnp                # + owning Process (may need sudo)
ss -tan                 # all TCP sockets, all states
ss -tan state established
ss -tan state time-wait
ss -tan '( sport = :8080 or dport = :8080 )'   # filter by port
ss -s                   # summary counts per state

netstat -tlnp           # older equivalent
lsof -i :8080           # who is using port 8080
lsof -iTCP -sTCP:LISTEN -n -P
```

Flags: `-t` TCP, `-u` UDP, `-l` listening, `-a` all, `-n` no name resolution, `-p` show process.

Example with the server running:

```bash
ss -tlnp | grep 8080
```

```text
LISTEN 0      10      0.0.0.0:8080      0.0.0.0:*    users:(("server",pid=2541973,fd=3))
```

The columns when using the command `ss -tlnp` are:
```text
State       Recv-Q       Send-Q                           Local Address:Port              Peer Address:Port      Process                                             
LISTEN      0            511                                  127.0.0.1:34669                  0.0.0.0:*          users:(("code",pid=1964616,fd=93))                 
LISTEN      0            4096                                 127.0.0.1:46657                  0.0.0.0:*          users:(("language_server",pid=1964953,fd=11))
.
.
.
```

* `0.0.0.0:8080` means all interfaces (`INADDR_ANY`); `127.0.0.1:8080` would mean loopback only.
* `Send-Q` (`10`) is the backlog passed to `listen(10)`. For a listening socket `Recv-Q` is the number of connections waiting for `accept()`.

How this maps to the code:

* After `listen()`: the server socket is in `LISTEN`.
* After `accept()`: a second socket appears in `ESTAB`; this is the `client` object.
* After a connection closes you may see `TIME-WAIT`. Restarting a server without `SO_REUSEADDR` can then fail with "Address already in use".

After a few client runs, the closed connections linger in `TIME-WAIT` (on the side that closed first, here the client):

```bash
ss -tan state time-wait '( sport = :8080 or dport = :8080 )'
```

```text
Recv-Q  Send-Q  Local Address:Port  Peer Address:Port
0       0       127.0.0.1:54674     127.0.0.1:8080
0       0       127.0.0.1:54662     127.0.0.1:8080
0       0       127.0.0.1:54656     127.0.0.1:8080
```

## 2. Talk to a server: `nc`, `curl`, `telnet`

```bash
nc localhost 8080                 # interactive TCP client
nc -N localhost 8080              # close the socket on stdin EOF
nc -l 9000                        # be a server on port 9000
nc -u localhost 9000              # UDP mode
nc -zv localhost 8000-8100        # scan a port range (-z no data, -v verbose)

curl -v telnet://localhost:8080   # raw TCP via curl
curl http://localhost:8080        # sends an HTTP request to the server
```

`curl http://localhost:8080` makes the server print the raw HTTP request (`GET / HTTP/1.1`, `Host:`, `User-Agent:` ...), **showing that HTTP is just text over a socket**.

`nc -l` is also handy to test the client without the real server:

```bash
nc -l 8080                  # terminal 1 (stop the real server first)
./build/client "Hello"      # terminal 2
```

```text
Hello
```

## 3. Watch packets: `tcpdump`

```bash
sudo tcpdump -i lo -n port 8080          # loopback traffic on port 8080
sudo tcpdump -i lo -n -A port 8080       # + payload as ASCII
sudo tcpdump -i lo -n -X port 8080       # + hex and ASCII
sudo tcpdump -i any -n port 8080 -c 20   # any interface, stop after 20 packets
```

Run it in a third terminal, then run `./build/client "Hello"`. TCP lifecycle flags:

| Flags  | Meaning |
|--------|---------|
| `[S]`  | SYN: client starts the handshake (`connect()`) |
| `[S.]` | SYN-ACK: server's kernel replies |
| `[.]`  | ACK: handshake done (`accept()` can return) |
| `[P.]` | PSH-ACK: data sent (`send()` / `recv()`) |
| `[F.]` | FIN: someone called `close()` (your `recv()` returns 0) |
| `[R.]` | RST: connection reset or refused |

Wireshark shows the same information; `tshark` is its CLI.

## 4. Watch system calls: `strace`

```bash
strace ./build/server                              # everything (noisy)
strace -e trace=network ./build/server             # socket-related calls only
strace -e trace=socket,bind,listen,accept,accept4,recvfrom,sendto,close ./build/server
strace -f -tt -e trace=network ./build/server      # follow threads, timestamps
sudo strace -p <PID> -e trace=network              # attach to a running server
```

Expect `socket(AF_INET, SOCK_STREAM, IPPROTO_IP) = 3`, `bind(3, {sin_port=htons(8080), ...})`, `listen(3, 10)`, then a blocked `accept(...)` until a client connects. `recv`/`send` may show as `recvfrom`/`sendto`, and `accept` as `accept4`; they are the same calls under their kernel names.

## 5. File descriptors and processes

A socket is just a file descriptor:

```bash
pgrep -a server
ls -l /proc/<PID>/fd         # sockets appear as socket:[inode]
lsof -p <PID>                # all open files and sockets
ulimit -n                    # max open fds per process
```

With the server idle, only the listening socket is open:

```bash
ls -l /proc/$(pgrep -x server)/fd | grep socket
```

```text
lrwx------ 1 user user 64 ... 3 -> socket:[26480852]
```

While a client is connected a second `socket:[...]` entry appears (fd 4) and disappears when it disconnects.

Stopping and cleaning up:

```bash
kill <PID>                   # SIGTERM
kill -9 <PID>                # force
pkill server
fuser -k 8080/tcp            # kill whatever holds port 8080
```

## 6. Network configuration and reachability

```bash
ip addr                      # interfaces and IPs
ip route                     # routing table
ping -c 3 localhost
traceroute example.com
dig example.com             # or: nslookup example.com
getent hosts localhost
cat /etc/hosts
```

`127.0.0.1` (loopback) is reachable only from your machine; your LAN address is reachable from other machines. `INADDR_ANY` binds all of them.

## 7. Port scanning with `nmap`

```bash
nmap localhost
nmap -p 8080 localhost
nmap -p 1-10000 localhost
```

Only scan your own machine or networks you have permission to test.

## 8. Kernel limits and settings

```bash
cat /proc/sys/net/core/somaxconn          # max backlog the kernel allows
sysctl net.ipv4.tcp_fin_timeout
sysctl net.ipv4.ip_local_port_range       # ephemeral port range for clients
cat /proc/net/tcp                         # raw kernel socket table
```

## Experiments

| Experiment | How |
|------------|-----|
| Handshake | `sudo tcpdump -i lo -n port 8080`, then `echo hi \| nc -N localhost 8080`. Find SYN, SYN-ACK, ACK, data and FIN. |
| Syscall map | `strace -e trace=network ./build/server`; match each line to the code. |
| Listen queue | Keep one client connected (the server is sequential), open several more `nc` connections, and watch `ss -tln` show `Recv-Q` grow. |
| TIME-WAIT | Run the client a few times, then `ss -tan state time-wait`. |
| Address in use | Start two servers; the second fails in `bind()`. Find the owner with `lsof -i :8080`. |
| Connection refused | Stop the server, run `nc localhost 8080`, and compare with `tcpdump` (RST reply). |
| FD leak | Watch `ls /proc/<PID>/fd \| wc -l` while connecting repeatedly. The RAII `Socket` should keep the count flat. |
