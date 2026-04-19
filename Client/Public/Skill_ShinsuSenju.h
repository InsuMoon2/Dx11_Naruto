#pragma once

#include "SkillObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class Skill_ShinsuSenju : public SkillObject
{
    GENERATED_BODY(Skill_ShinsuSenju)

public:
    explicit Skill_ShinsuSenju(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_ShinsuSenju(const Skill_ShinsuSenju& rhs);
    virtual ~Skill_ShinsuSenju() = default;
    
public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    void    Spawn_ShinsuSenju(const Vec3 targetPos);
    void    Spawn_Arm(const Vec3& targetDirection);

private:
    Shared<Shader> _shader;
    Shared<Model>  _model;

private:
    float   _burstInterval = 0.14f;
    int32   _maxBurstCount = 5;

    float   _delayElapsed = 0.f;
    float   _burstElapsed = 0.f;

    int32   _burstCount = 0;
    bool    _isBurstFinished = false;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
