#pragma once
#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_StretchingMesh_End : public AnimNotify
{
    GENERATED_BODY(AN_StretchingMesh_End)

public:
    string Get_TypeName() const override { return "AN_StretchingMesh_End"; }

public:
    virtual void Execute(const FAnimNotifyContext& context) override;
};

NS_END
