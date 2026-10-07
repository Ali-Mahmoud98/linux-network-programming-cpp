# Summary

أكيد. خلينا نختصر **الـ system الحالي كله** في pseudocode صغير، بحيث يبقى عندك mental model قبل ما نكمل.

```text
START

    create TCP listening socket
    make socket NON-BLOCKING

    bind()
    listen()

    create epoll instance

    add server socket to epoll
        events = EPOLLIN | EPOLLET


    LOOP FOREVER

        events = epoll_wait()


        FOR each ready event

            IF event belongs to server socket

                WHILE true
                    clientFd = accept()

                    IF EAGAIN
                        BREAK

                    make clientFd NON-BLOCKING

                    create ClientConnection(clientFd)

                    add clientFd to epoll
                        events = EPOLLIN | EPOLLET

                    store client


            ELSE

                client = find client


                IF connection error/disconnect

                    remove from epoll
                    close client
                    remove client


                IF EPOLLIN

                    WHILE true

                        data = recv()

                        IF data > 0

                            append data to client.outputBuffer

                            enable EPOLLOUT

                        ELSE IF data == 0

                            close client
                            BREAK

                        ELSE IF EAGAIN

                            BREAK


                IF EPOLLOUT

                    WHILE client has pending output

                        bytesSent = send(
                            outputBuffer + outputOffset
                        )

                        IF bytesSent > 0

                            outputOffset += bytesSent

                        ELSE IF EAGAIN

                            BREAK

                        ELSE

                            close client
                            BREAK


                    IF outputBuffer is completely sent

                        clear outputBuffer
                        outputOffset = 0

                        disable EPOLLOUT


END
```

### والـ mental model في سطر واحد

```text
                 epoll_wait()
                     │
          ┌──────────┴──────────┐
          │                     │
       EPOLLIN               EPOLLOUT
          │                     │
        recv()                send()
          │                     │
          ▼                     ▼
   outputBuffer            outputOffset
          │                     │
          └──────────┬──────────┘
                     │
                EAGAIN
                     │
                     ▼
                back to epoll
```

وأهم 3 قواعد وصلنالها:

```text
EPOLLIN
   → drain recv() until EAGAIN

EPOLLOUT
   → drain send() until EAGAIN

EPOLLOUT
   → ON  only when outputBuffer has data
   → OFF when outputBuffer becomes empty
```

وده حاليًا هو **الـ core event-driven TCP server model** اللي هنحاول بعد كده نفهم إزاي Boost.Asio بيعمل abstraction فوقه.
