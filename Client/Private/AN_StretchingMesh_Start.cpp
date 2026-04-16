#include "pch.h"
#include "AN_StretchingMesh_Start.h"
#include "AnimNotify_Factory.h"
#include "StretchingMeshEffect.h"
#include "Model.h"
#include "Transform.h"
#include "SkillComponent.h"
#include "GameObject.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(AN_StretchingMesh_Start)
IMPLEMENT_REFLECTION(AN_StretchingMesh_Start)

bool AN_StretchingMesh_Start::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_StretchingMesh_Start";

    PROPERTY_STRING_JSON("이펙트 에셋명", "effect_name", _effectAssetName);
    PROPERTY_STRING_JSON("시작 뼈 이름", "bone_name", _boneName);
    PROPERTY_FLOAT_JSON("원본 길이", "mesh_length", _originalLength, 0.1f, 1000.f);
    PROPERTY_VEC3_JSON("굵기(두께)", "thickness", _thickness, 0.1f);
    PROPERTY_VEC3_JSON("회전 오프셋", "rotation_offset", _rotationOffset, 1.f);

    PROPERTY_VEC3_JSON("로컬 위치 증분", "local_offset_step", _localOffsetStep, 0.01f);
    PROPERTY_VEC3_JSON("회전 증분", "rotation_offset_step", _rotationOffsetStep, 1.f);
    PROPERTY_VEC3_JSON("두께 증분", "thickness_step", _thicknessStep, 0.01f);
    PROPERTY_INT_JSON("스폰 개수", "spawn_count", _spawnCount, 1, 10);

    return true;
}

void AN_StretchingMesh_Start::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner || !context.model)
        return;

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    if (!skillCom)
        return;

    skillCom->Destroy_AllStretchingMeshes();

    Matrix boneWorldMatrix = context.owner->Get_Transform()->Get_WorldMatrix();
    if (const Matrix* socketMatrix = context.model->Get_SocketBoneMatrixPtr(_boneName))
        boneWorldMatrix = (*socketMatrix) * boneWorldMatrix;

    Vec3 startWorldPos = boneWorldMatrix.Translation();

    const int32 spawnCount = max(1, _spawnCount);

    for (int32 i = 0; i < spawnCount; ++i)
    {
        const float centeredIndex =
            static_cast<float>(i) - (static_cast<float>(spawnCount - 1) * 0.5f);

        StretchingMeshEffect::FStretchingMeshDesc desc{};
        desc.position           = startWorldPos;
        desc.effectAssetName    = _effectAssetName;
        desc.meshOriginalLength = _originalLength;

        desc.thickness = Vec3(
            max(0.01f, _thickness.x + _thicknessStep.x * centeredIndex),
            max(0.01f, _thickness.y + _thicknessStep.y * centeredIndex),
            max(0.01f, _thickness.z + _thicknessStep.z * centeredIndex));

        desc.rotationOffset = _rotationOffset + (_rotationOffsetStep * centeredIndex);

        desc.localOffset = _localOffsetStep * centeredIndex;

        desc.ownerObj      = context.owner->GetSharedPtr<GameObject>();
        desc.trackBoneName = _boneName;

        auto spawned = GAME->Clone_And_Add_GameObject(
            0,
            Protocol::OBJECT_TYPE_STRETCHING_MESH_EFFECT,
            GAME->Current_Level(),
            TEXT("Layer_Effect"),
            &desc
        );

        if (spawned)
        {
            skillCom->Add_ActiveStretchingMesh(spawned);
        }
    }
}

NS_END
