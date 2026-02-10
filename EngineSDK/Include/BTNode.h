#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;
class Blackboard;

class ENGINE_DLL BTNode : public Base
{
public:
    explicit BTNode();
    virtual ~BTNode();

public:
    virtual void Initialize() {};
    virtual EBTNodeResult Update(float timeDelta);

    // 종료 (Success/Fail/Abort 시 호출)
    virtual void OnTerminate(EBTNodeResult result) {};

    // 블랙보드 세팅
    void Set_Blackboard(Shared<Blackboard> blackboard) { _blackboard = blackboard; }
    void Set_Owner(Shared<GameObject> owner) { _owner = owner; }

protected:
    Shared<Blackboard>  _blackboard;
    Weak<GameObject>    _owner;

};

NS_END
