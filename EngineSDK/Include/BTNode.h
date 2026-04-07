#pragma once

#include "Base.h"
#include "Property_Types.h"

NS_BEGIN(Engine)

class GameObject;
class Blackboard;

class ENGINE_DLL BTNode : public Base
{
public:
    explicit BTNode();
    explicit BTNode(const BTNode& rhs);
    virtual ~BTNode();

public:
    virtual void Initialize();
    virtual EBTNodeResult Update(float timeDelta);

public:
    virtual const FClassReflectionInfo& GetReflectionInfo() const
    {
        static FClassReflectionInfo empty;
        return empty;
    }

public:
    // 종료 (Success/Fail/Abort 시 호출)
    virtual void OnTerminate(EBTNodeResult result) {};

    // 블랙보드 세팅
    virtual void  Set_Blackboard(Shared<Blackboard> blackboard) { _blackboard = blackboard; }
    virtual void  Set_Owner(Shared<GameObject> owner) { _owner = owner; }

    EBTNodeResult Get_LastResult() const { return _lastResult; }
    int           Get_DebugId() const { return _debugId; }
    void          Set_DebugId(int id) { _debugId = id; }

    string        Get_Name() const { return _name; }
    void          Set_Name(const string& name) { _name = name; }

    virtual void  OnDraw_Inspector() {};
    virtual json  Serialize_ToJson();
    virtual void  Deserialize_FromJson(const json& data);

    virtual void  Gather_NodeResults(map<int, EBTNodeResult>& outResults);

protected:
    Weak<Blackboard>  _blackboard;
    Weak<GameObject>  _owner;

    EBTNodeResult     _lastResult = EBTNodeResult::Failed;
    int               _debugId = -1; // 에디터 노드 ID랑 매칭

    string            _name = "";

public:
    virtual Shared<BTNode> Clone() = 0;

};

NS_END
