#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class EffectComponent;
NS_END

NS_BEGIN(Client)

class WireMeshEffect final : public GameObject
{
    GENERATED_BODY(WireMeshEffect)

public:
    struct FWireMeshEffectDesc : public FGameObjectDesc
    {
        string effectAssetName = "";
        Weak<GameObject> ownerObj;
        string trackBoneName = "R_Hand_Weapon_cnt_tr";
        Vec3 targetPosition = Vec3::Zero;
        float meshOriginalLength = 1.f;
        Vec3 thickness = Vec3(1.f, 1.f, 1.f);
        Vec3 rotationOffset = Vec3::Zero;
        Vec3 localOffset = Vec3::Zero;
    };

public:
    explicit WireMeshEffect(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit WireMeshEffect(const WireMeshEffect& rhs);
    ~WireMeshEffect() override = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override { return S_OK; }

    bool Should_ExcludeFromEditorSnapshot() const override { return true; }

public:
    void Set_TargetPosition(const Vec3& targetPosition);

private:
    HRESULT Ready_Components(const string& effectAssetName);
    bool    Try_GetHandWorldPosition(Vec3& outHandWorldPosition) const;

private:
    Shared<EffectComponent> _effectCom = nullptr;

    Weak<GameObject> _ownerObj;
    string _trackBoneName = "R_Hand_Weapon_cnt_tr";

    Vec3 _targetPosition = Vec3::Zero;
    Vec3 _spawnWorldPosition = Vec3::Zero;

    float _meshOriginalLength = 1.f;
    Vec3 _thickness = Vec3(1.f, 1.f, 1.f);
    Vec3 _rotationOffset = Vec3::Zero;
    Vec3 _localOffset = Vec3::Zero;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
