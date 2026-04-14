#include "pch.h"
#include "AN_SpawnParticle.h"
#include "AnimNotify_Factory.h"
#include "AttachedEffectObject.h"
#include "GameObject.h"
#include "Model.h"

REGISTER_ANIM_NOTIFY(AN_SpawnParticle)
IMPLEMENT_REFLECTION(AN_SpawnParticle)

bool AN_SpawnParticle::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_SpawnParticle";
    info.properties.clear();

    PROPERTY_STRING_JSON("이펙트 이름", "effect_name", _effectAssetName);
    PROPERTY_STRING_JSON("장착 뼈", "bone_name", _boneName);
    PROPERTY_VEC3_JSON("로컬 오프셋", "local_offset", _localOffset, 0.1f);
    PROPERTY_VEC3_JSON("로컬 회전", "local_rotation", _localRotation, 1.f);
    PROPERTY_VEC3_JSON("로컬 스케일", "local_scale", _localScale, 0.1f);
    PROPERTY_BOOL_JSON("루프 강제", "loop_override", _loopOverride);

    PROPERTY_BOOL_JSON("뼈 추적 Attach", "attach_to_bone", _attachToBone);
    PROPERTY_BOOL_JSON("뼈 초기 위치만 사용", "use_initial_bone_transform", _useInitialBoneTransform);

    return true;
}

void AN_SpawnParticle::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner || !context.model)
        return;

    // 중복 방지
    if (context.wrapped)
        return;

    // 에셋 이름 없는거 방지
    if (_effectAssetName.empty())
        return;

     if (_attachToBone || _useInitialBoneTransform)
    {
        if (_boneName.empty())
            return;

        const Matrix* socketMatrix = context.model->Get_SocketBoneMatrixPtr(_boneName);
        if (!socketMatrix)
            return;
    }

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

    if (_attachToBone)
    {
        attachedEffect->Attach_To_Bone(
            context.model,
            context.owner->Get_Transform(),
            _boneName,
            _localOffset,
            _localRotation,
            _localScale);

        return;
    }

    // 초기 위치 유지
    if (_useInitialBoneTransform)
    {
        const Matrix* socketMatrix = context.model->Get_SocketBoneMatrixPtr(_boneName);
        if (!socketMatrix)
            return;

        Matrix boneWorldMatrix = (*socketMatrix) * context.owner->Get_Transform()->Get_WorldMatrix();

        attachedEffect->Apply_InitialTransform(
            boneWorldMatrix,
            _localOffset,
            _localRotation,
            _localScale);

        return;
    }

    Matrix ownerWorldMatrix = context.owner->Get_Transform()->Get_WorldMatrix();

    attachedEffect->Apply_InitialTransform(
        ownerWorldMatrix,
        _localOffset,
        _localRotation,
        _localScale);
        
}
