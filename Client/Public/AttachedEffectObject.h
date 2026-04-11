#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class EffectComponent;
class Model;
class Transform;
NS_END

NS_BEGIN(Client)

class AttachedEffectObject final : public GameObject
{
    GENERATED_BODY(AttachedEffectObject)

public:
    struct FAttachedEffectObjectDesc : public GameObject::FGameObjectDesc
    {
        string effectAssetName = "";
        bool   loopOverride = false;
    };

public:
    explicit AttachedEffectObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit AttachedEffectObject(const AttachedEffectObject& rhs);
    virtual ~AttachedEffectObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    bool    Should_ExcludeFromEditorSnapshot() const override { return true; }

public:
    Shared<EffectComponent> Get_EffectComponent() const { return _effectCom; }

    void Sync_AttachedTransform(
        const Matrix& boneWorldMatrix,
        const Vec3& localOffset,
        const Vec3& localRotation,
        const Vec3& localScale);

    void Stop_AttachedEffect();

     void Attach_To_Bone(
        Model* targetModel, 
        Weak<Transform> targetTransform,
        const string& boneName,
        const Vec3& localOffset,
        const Vec3& localRotation,
        const Vec3& localScale);

private:
    HRESULT Ready_Components();

private:
    Shared<EffectComponent> _effectCom;

    string _effectAssetName = "";
    bool   _loopOverride = false;

    // 이팩트 재생이 끝나면 자동으로 삭제될지
    bool _autoDestroyOnFinish = true;
    bool _isTrackingBone = false;

    Model* _targetModel = nullptr;
    Weak<Transform> _targetTransform;

    string _targetBoneName = "";

    Vec3 _targetLocalOffset = Vec3::Zero;
    Vec3 _targetLocalRotation = Vec3::Zero;
    Vec3 _targetLocalScale = Vec3::One;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
