#pragma once

#include "SkillObject_Projectile.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class AnimationStateComponent;

class Skill_WindmillShuriken final : public SkillObject_Projectile
{
    GENERATED_BODY(Skill_WindmillShuriken)

public:
    explicit Skill_WindmillShuriken(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_WindmillShuriken(const Skill_WindmillShuriken& rhs);
    virtual ~Skill_WindmillShuriken() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    void    Launch(const Vec3& direction) override;

private:
    HRESULT Ready_Components();
    HRESULT Ready_AnimState();

    HRESULT Bind_ShaderResources();

private:
    Shared<Shader> _shader;
    Shared<Model>  _model;

    Shared<AnimationStateComponent> _animState;

private:
    string _animStateKey        = "WindmillShuriken";
    string _startAnimationName  = "SK_WEP_WindmillShuriken|WindmillShuriken_Rot_Start";
    string _loopAnimationName   = "SK_WEP_WindmillShuriken|WindmillShuriken_Rot_Loop";

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
