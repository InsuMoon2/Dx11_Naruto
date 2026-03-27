#pragma once

#include "BTTask.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTTask_SetAnimState : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_SetAnimState)

public:
    explicit BTTask_SetAnimState(const string& stateName = "Idle");
    explicit BTTask_SetAnimState(const BTTask_SetAnimState& rhs);
    virtual ~BTTask_SetAnimState() = default;

public:
    void            Initialize() override;
    EBTNodeResult   Update(float timeDelta) override;

    // 에디터 인스팩터에 표시 -> 구조 변경
    //void            OnDraw_Inspector() override;

    json            Serialize_ToJson() override;
    void            Deserialize_FromJson(const json& data) override;

private:
    string          _stateName = "Idle";

public:
    static Shared<BTTask_SetAnimState> Create();
    Shared<BTNode>  Clone() override;

};

NS_END
