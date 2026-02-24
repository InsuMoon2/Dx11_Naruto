#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"

NS_BEGIN(Server)

GameSessionManager GSessionManager;

NS_END

void GameSessionManager::Add(shared_ptr<GameSession> session)
{
    WRITE_LOCK;
    _sessions.insert(session);

}

void GameSessionManager::Remove(shared_ptr<GameSession> session)
{
    WRITE_LOCK;
    _sessions.erase(session);
}

void GameSessionManager::Broadcast(shared_ptr<SendBuffer> sendBuffer)
{
    WRITE_LOCK;
    for (shared_ptr<GameSession> session : _sessions)
    {
        session->Send(sendBuffer);
    }
}
