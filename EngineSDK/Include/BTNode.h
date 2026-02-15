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
    virtual void Set_Blackboard(Shared<Blackboard> blackboard) { _blackboard = blackboard; }
    void Set_Owner(Shared<GameObject> owner) { _owner = owner; }

    EBTNodeResult Get_LastResult() const { return _lastResult; }
    int           Get_DebugId() const { return _debugId; }
    void          Set_DebugId(int id) { _debugId = id; }

protected:
    Weak<Blackboard>  _blackboard;
    Weak<GameObject>  _owner;

    EBTNodeResult     _lastResult = EBTNodeResult::Failed;
    int               _debugId = -1; // 에디터 노드 ID랑 매칭

public:
    virtual Shared<BTNode> Clone() = 0;

};

NS_END
