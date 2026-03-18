#pragma once

#include "Base.h"
#include "AnimNotify_Types.h"

NS_BEGIN(Engine)

class ENGINE_DLL AnimNotify : public Base
{
public:
    AnimNotify() = default;
    virtual ~AnimNotify() = default;

public:
    virtual string Get_TypeName() const = 0;
    virtual void Execute(const FAnimNotifyContext& context) = 0;

    virtual json Serialize_Payload() const;
    virtual void Deserialize_Payload(const json& payload);
};

NS_END
