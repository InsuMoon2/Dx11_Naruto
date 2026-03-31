#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class Collider;
NS_END

class SkillObject : public GameObject
{
    GENERATED_BODY(SkillObject)

public:
    struct FSkillObjectDesc : public FGameObjectDesc
    {
        Protocol::ComponentID colliderType = Protocol::COMPONENT_TYPE_COLLIDER_SPHERE;

        float   colliderRadius = 1.f;                       
        Vec3    colliderExtents = Vec3(0.5f, 0.5f, 0.5f);

        Vec3    spawnPosition = Vec3::Zero; 
        Vec3    spawnRotation = Vec3::Zero; 
        Vec3    scale         = Vec3::One;  
        float   lifetime      = 2.f;

        int32   ownerSkillId  = 0;          
    };

public:
    explicit SkillObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkillObject(const SkillObject& rhs);
    virtual ~SkillObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;

protected:
    HRESULT Ready_Components(const FSkillObjectDesc& desc);

protected:
    Shared<Collider> _collider;

    float   _lifetime = 2.f;        // 수명 시간     
    float   _elapsedTime = 0.f;     // 스폰 후 경과 시간

    int32   _ownerSkillId = 0;      

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
    
};

