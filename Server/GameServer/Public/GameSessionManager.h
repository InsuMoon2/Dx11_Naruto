#pragma once


NS_BEGIN(Server)

class GameSession;

class GameSessionManager
{
public:
    GameSessionManager() = default;
    ~GameSessionManager() = default;

public:
    void Add(shared_ptr<GameSession> session);
    void Remove(shared_ptr<GameSession> session);
    void Broadcast(shared_ptr<SendBuffer> sendBuffer);

private:
    USE_LOCK;
    set<shared_ptr<GameSession>> _sessions;
};

extern GameSessionManager GSessionManager;

NS_END
