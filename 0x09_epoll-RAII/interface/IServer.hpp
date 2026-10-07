#pragma once

class IServer
{
public:
    virtual ~IServer() = default;

    virtual void start() = 0;
};
