#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_Test : public AnimNotifyState
{
public:
    string Get_TypeName() const override;

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context) override;
    void On_End(const FAnimNotifyContext& context) override;

public:
    json Serialize_Payload() const override;
    void Deserialize_Payload(const json& payload) override;
    void Free() override;
};

NS_END
