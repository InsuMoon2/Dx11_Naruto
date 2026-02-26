#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Light;

class Light_Manager : public Base
{
public:
    explicit Light_Manager();
    virtual ~Light_Manager() = default;

public:
    const FLightDesc* Get_LightDesc(uint32 index);
    HRESULT           Add_Light(const FLightDesc& desc);
    void              Clear_Lights();

private:
    vector<Shared<Light>> _lights;

public:
    static Unique<Light_Manager> Create();
    void Free() override;

};

NS_END
