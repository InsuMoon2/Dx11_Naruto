#include "pch.h"
#include "GameRoom.h"
#include "GameSession.h"
#include "Server_PacketHandler.h"

#include "GameObject.h"
#include "Player.h"
#include "Monster.h"

NS_BEGIN(Server)
Shared<GameRoom> GRoom = make_shared<GameRoom>();
NS_END

GameRoom::GameRoom()
{
}

GameRoom::~GameRoom()
{
}

void GameRoom::Enter_GameRoom(
    Shared<GameSession> session,
    float spawnX,
    float spawnY,
    float spawnZ,
    float rotY)
{
    auto player = Player::Create();
    player->Set_Session(session);
    session->Set_PlayerId(player->Get_ObjectID());

    auto* protoPos = player->info.mutable_pos();
    protoPos->set_x(spawnX);
    protoPos->set_y(spawnY);
    protoPos->set_z(spawnZ);
    player->info.set_rot_y(rotY);

    {
        SendBufferRef sendBuffer = Server_PacketHandler::Make_S_MyPlayer(player->info);
        session->Send(sendBuffer);
    }

    {
        Protocol::S_AddObject pkt;

        for (auto& [id, existingPlayer] : _players)
        {
            Protocol::ObjectInfo* info = pkt.add_objects();
            *info = existingPlayer->info;
        }

        for (auto& [id, existingMonster] : _monsters)
        {
            Protocol::ObjectInfo* info = pkt.add_objects();
            *info = existingMonster->info;
        }

        if (pkt.objects_size() > 0)
        {
            SendBufferRef sendBuffer = Server_PacketHandler::Make_S_AddObject(pkt);
            session->Send(sendBuffer);
        }
    }

    Add_Player(player);
}

void GameRoom::Leave_GameRoom(Shared<GameSession> session)
{
    if (session == nullptr)
        return;

    Shared<Player> player = Find_Player(session->Get_PlayerId());
    if (player == nullptr)
        return;

    uint64 id = player->Get_ObjectID();
    Remove_Player(id);

    cout << "[Room] Player " << id
        << " Left (" << _players.size() << " players)" << endl;

}

void GameRoom::Handle_C_Move(Protocol::C_Move& pkt)
{
    uint64 id = pkt.info().objectid();

    Shared<GameObject> gameObject = Find_Player(id);
    if (gameObject == nullptr)
        return;

    // 서버 측 위치 갱신
    gameObject->info = pkt.info();
    gameObject->info.set_objectid(id);

    // 모든 플레이어들에게 이동 패킷 전달
    {
        SendBufferRef sendBuffer = Server_PacketHandler::Make_S_Move(gameObject->info);
        Broadcast(sendBuffer);
    }

}

void GameRoom::Broadcast(SendBufferRef sendBuffer)
{
    for (auto& [id, player] : _players)
    {
        if (player->Get_Session())
            player->Get_Session()->Send(sendBuffer);
    }
}

void GameRoom::Update()
{
    for (auto& [id, player] : _players)
    {
        player->Update();
    }

    for (auto& [id, monster] : _monsters)
    {
        monster->Update();
    }
}
