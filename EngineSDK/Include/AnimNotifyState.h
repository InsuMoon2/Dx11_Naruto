#pragma once

#include "Base.h"
#include "AnimNotify_Types.h"

NS_BEGIN(Engine)

class ENGINE_DLL AnimNotifyState : public Base
{
public:
    AnimNotifyState() = default;
    virtual ~AnimNotifyState() = default;

public:
    virtual string Get_TypeName() const = 0;

    virtual void On_Begin(const FAnimNotifyContext& context) = 0;
    virtual void On_Tick(const FAnimNotifyContext& context) = 0;
    virtual void On_End(const FAnimNotifyContext& context) = 0;

    virtual json Serialize_Payload() const;
    virtual void Deserialize_Payload(const json& payload);

public:
    virtual FClassReflectionInfo& Get_ReflectionInfo()
    {
        static FClassReflectionInfo emptyInfo{};
        return emptyInfo;
    }
};

NS_END
