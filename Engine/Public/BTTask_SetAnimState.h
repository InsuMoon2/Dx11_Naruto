#pragma once

#include "BTTask.h"

NS_BEGIN(Engine)

class ENGINE_DLL BTTask_SetAnimState : public BTTask
{
public:
    explicit BTTask_SetAnimState(const string& stateName = "Idle");
    explicit BTTask_SetAnimState(const BTTask_SetAnimState& rhs);
    virtual ~BTTask_SetAnimState() = default;

public:
    void            Initialize() override;
    EBTNodeResult   Update(float timeDelta) override;

    // 에디터 인스팩터에 표시
    void            OnDraw_Inspector() override;

    json            Serialize_ToJson() override;
    void            Deserialize_FromJson(const json& data) override;

private:
    // 몬스터는 상태 이름을 string으로 관리하는거 더 편할거같은데, 일단 진행을 해봐야 할거같음
    string          _stateName = "Idle";

public:
    Shared<BTNode>  Clone() override;

};

NS_END
