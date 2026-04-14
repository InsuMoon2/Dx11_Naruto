#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class EffectComponent;
NS_END

NS_BEGIN(Client)

class StretchingMeshEffect final : public GameObject
{
    GENERATED_BODY(StretchingMeshEffect)

public:
    struct FStretchingMeshDesc : public FGameObjectDesc
    {
        string              effectAssetName = "";
        float               meshOriginalLength = 1.0f; 
        Vec3                thickness = Vec3(1.f, 1.f, 1.f);

        Vec3                rotationOffset = Vec3::Zero;
        Vec3                localOffset = Vec3::Zero;

        // 이팩트가 소스 타겟 뼈를 추적하기 위해
        Weak<GameObject> ownerObj;
        string trackBoneName = "";
    };

public:
    explicit StretchingMeshEffect(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit StretchingMeshEffect(const StretchingMeshEffect& rhs);
    virtual ~StretchingMeshEffect() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    
    virtual HRESULT Render() override { return S_OK; }

   void Update_TargetPosition(const Vec3& targetPos)
    {
        _currentTargetPos = targetPos;
    }

private:
    HRESULT Ready_Components(const string& effectAssetName);

private:
    Shared<EffectComponent> _effectCom = nullptr; 

    float                   _meshOriginalLength = 1.0f;
    Vec3                    _thickness = Vec3(1.f, 1.f, 1.f);
    Vec3                    _rotationOffset = Vec3::Zero;
    Vec3                    _localOffset = Vec3::Zero; 

    Vec3                    _spawnWorldPos = Vec3::Zero;
    Vec3                    _currentTargetPos = Vec3::Zero;
    Vec3                    _ownerSpawnWorldPos = Vec3::Zero;

    Weak<GameObject>        _ownerObj;
    string                  _trackBoneName = "";

public:
    static  Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
