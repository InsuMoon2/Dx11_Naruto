#pragma once

#include "GameObject.h"

NS_BEGIN(Server)

class GameSession;

class Player : public GameObject
{
public:
    explicit Player();
    virtual ~Player();

public:
    Shared<GameSession>  Get_Session() const { return _session; }
    void                 Set_Session(Shared<GameSession> session) { _session = session; }

private:
    Shared<GameSession> _session;

public:
    static Shared<Player> Create();

};

NS_END
