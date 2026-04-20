#pragma once

#include "SkillObject_Projectile.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class Skill_ShinsuSenju_Arm : public SkillObject_Projectile
{
    GENERATED_BODY(Skill_ShinsuSenju_Arm)

public:
    explicit Skill_ShinsuSenju_Arm(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_ShinsuSenju_Arm(const Skill_ShinsuSenju_Arm& rhs);
    virtual ~Skill_ShinsuSenju_Arm() = default;
    
public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    void    Apply_InitialRotation(const Vec3& launchDirection);

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    void Spawn_Particle(const string& assetName, int32 spawnIndex);

private:
    Shared<Shader> _shader;
    Shared<Model>  _model;

private:
    Vec3 _targetPoint = Vec3::Zero;
    bool _hasTargetPoint = false;
    float _stopDistance = 1.0f;

    bool _isStop = false;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
