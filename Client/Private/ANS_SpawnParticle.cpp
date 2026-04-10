#include "pch.h"
#include "ANS_SpawnParticle.h"
#include "AnimNotify_Factory.h"
#include "AttachedEffectObject.h"
#include "GameObject.h"
#include "Model.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_SpawnParticle)
IMPLEMENT_REFLECTION(ANS_SpawnParticle)

bool ANS_SpawnParticle::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_SpawnParticle";
    info.properties.clear();

    PROPERTY_STRING_JSON("이펙트 이름", "effect_name", _effectAssetName);
    PROPERTY_STRING_JSON("장착 뼈", "bone_name", _boneName);
    PROPERTY_VEC3_JSON("로컬 오프셋", "local_offset", _localOffset, 0.1f);
    PROPERTY_VEC3_JSON("로컬 회전", "local_rotation", _localRotation, 1.f);
    PROPERTY_VEC3_JSON("로컬 스케일", "local_scale", _localScale, 0.1f);
    PROPERTY_BOOL_JSON("루프 강제", "loop_override", _loopOverride);

    return true;
}

void ANS_SpawnParticle::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner || !context.model)
        return;

    if (_effectAssetName.empty())
        return;

    AttachedEffectObject::FAttachedEffectObjectDesc desc{};
    desc.effectAssetName = _effectAssetName;
    desc.loopOverride = _loopOverride;

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_ATTACHED_EFFECT,
        GAME->Current_Level(),
        TEXT("Layer_Effect"),
        &desc);

    auto attachedEffect = dynamic_pointer_cast<AttachedEffectObject>(spawned);
    if (!attachedEffect)
        return;

    attachedEffect->Set_Owner(context.owner->GetSharedPtr<GameObject>());

    Matrix boneWorldMatrix = Matrix::Identity;
    if (Try_BuildBoneWorldMatrix(context, _boneName, boneWorldMatrix))
    {
        attachedEffect->Sync_AttachedTransform(
            boneWorldMatrix,
            _localOffset,
            _localRotation,
            _localScale);
    }

    _attachedEffect = attachedEffect;
}

void ANS_SpawnParticle::On_Tick(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner || !context.model)
        return;

    auto attachedEffect = _attachedEffect.lock();
    if (!attachedEffect || attachedEffect->Is_Destroy())
        return;

    Matrix boneWorldMatrix = Matrix::Identity;
    if (!Try_BuildBoneWorldMatrix(context, _boneName, boneWorldMatrix))
        return;

    attachedEffect->Sync_AttachedTransform(
        boneWorldMatrix,
        _localOffset,
        _localRotation,
        _localScale);

}

void ANS_SpawnParticle::On_End(const FAnimNotifyContext& context)
{
    UNREFERENCED_PARAMETER(context);

    auto attachedEffect = _attachedEffect.lock();
    if (attachedEffect)
    {
        attachedEffect->Stop_AttachedEffect();
    }

    _attachedEffect.reset();
}

bool ANS_SpawnParticle::Try_BuildBoneWorldMatrix(
    const FAnimNotifyContext& context,
    const string& boneName,
    Matrix& outBoneWorldMatrix)
{
    if (!context.owner || !context.model)
        return false;

    const Matrix* boneMatrix = context.model->Get_SocketBoneMatrixPtr(boneName);
    if (!boneMatrix)
        return false;

    auto ownerTransform = context.owner->Get_Transform();
    if (!ownerTransform)
        return false;

    outBoneWorldMatrix = (*boneMatrix) * ownerTransform->Get_WorldMatrix();

    return true;
}
