#pragma once

#include "IServer.hpp"
#include "ClientConnection.hpp"

#include <memory>
#include <unordered_map>

class EpollServer : public IServer
{
public:
    explicit EpollServer(int port);

    ~EpollServer() override;

    EpollServer(const EpollServer&) = delete;
    EpollServer& operator=(const EpollServer&) = delete;

    void start() override;

private:
    bool setup();

    bool setNonBlocking(int fd);

    void acceptClients();

    void handleClientRead(
        ClientConnection& client
    );

    void handleClientWrite(
        ClientConnection& client
    );

    void closeClient(int fd);

    void updateClientEvents(
        ClientConnection& client
    );

private:
    int m_port;

    int m_serverFd{-1};

    int m_epollFd{-1};

    std::unordered_map<
        int,
        std::unique_ptr<ClientConnection>
    > m_clients;
};
