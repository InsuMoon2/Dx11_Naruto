#pragma once

NS_BEGIN(Server)

class Player;
class Monster;
class GameRoom;

class GameObject
{
public:
    explicit GameObject() {}
    virtual ~GameObject() {}

public:
    virtual void Update();

public:
    uint64  Get_ObjectID()  const { return info.objectid(); }
    void    Set_ObjectID(uint64 id) { info.set_objectid(id); }
    void    BroadcastMove();

public:
    Protocol::ObjectInfo    info;
    Shared<GameRoom>        room;

protected:
    static atomic<uint64> s_idGenerator;

};

NS_END
