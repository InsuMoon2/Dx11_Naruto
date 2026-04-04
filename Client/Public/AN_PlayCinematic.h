#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_PlayCinematic : public AnimNotify
{
    GENERATED_BODY(AN_PlayCinematic)

public:
    string Get_TypeName() const override;
    void   Execute(const FAnimNotifyContext& context) override;

public:
    json   Serialize_Payload() const override;
    void   Deserialize_Payload(const json& payload) override;

private:
    string _sequenceName = "";

    // 입력을 막을건지
    bool   _blockGameInput = false;
};

NS_END
