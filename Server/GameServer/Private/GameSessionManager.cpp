#include "pch.h"
#include "GameSessionManager.h"
#include "GameSession.h"

GameSessionManager GSessionManager;

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
