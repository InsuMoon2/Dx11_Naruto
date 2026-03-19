#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_Test final : public AnimNotify
{
public:
    string Get_TypeName() const override;
    void Execute(const FAnimNotifyContext& context) override;

public:
    json Serialize_Payload() const override;
    void Deserialize_Payload(const json& payload) override;
    void Free() override;
};

NS_END
