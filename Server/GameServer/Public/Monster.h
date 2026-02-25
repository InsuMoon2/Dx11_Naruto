#pragma once

#include "GameObject.h"

NS_BEGIN(Server)

class GameSession;

class Monster : public GameObject
{
public:
    explicit Monster();
    virtual ~Monster();

public:
    Shared<GameSession>  Get_Session() const { return _session; }
    void                 Set_Session(Shared<GameSession> session) { _session = session; }

private:
    Shared<GameSession> _session;

public:
    static Shared<Monster> Create();
};

NS_END
