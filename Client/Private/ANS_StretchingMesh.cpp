#include "pch.h"
#include "ANS_StretchingMesh.h"
#include "AnimNotify_Factory.h"
#include "StretchingMeshEffect.h"
#include "Model.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY_STATE(ANS_StretchingMesh)
IMPLEMENT_REFLECTION(ANS_StretchingMesh)

bool ANS_StretchingMesh::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_StretchingMesh";

    PROPERTY_STRING_JSON("이펙트 에셋명", "effect_name", _effectAssetName);
    PROPERTY_STRING_JSON("장착 본 이름",  "bone_name",   _boneName);
    PROPERTY_FLOAT_JSON("원본 길이",      "mesh_length", _originalLength, 0.1f, 10.f);
    PROPERTY_VEC3_JSON("굵기(두께)",      "thickness",   _thickness, 0.1f);

    return true;
}

void ANS_StretchingMesh::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner || !context.model)
        return;

    const Matrix* socketMatrix = context.model->Get_SocketBoneMatrixPtr(_boneName);
    if (!socketMatrix) return;

    Matrix boneWorldMatrix = (*socketMatrix) * context.owner->Get_Transform()->Get_WorldMatrix();
    Vec3 startWorldPos = boneWorldMatrix.Translation();

    StretchingMeshEffect::FStretchingMeshDesc desc{};
    desc.position           = startWorldPos;
    desc.effectAssetName    = _effectAssetName;               
    desc.meshOriginalLength = _originalLength;
    desc.thickness          = _thickness;

    auto spawend = GAME->Clone_And_Add_GameObject(
        0, 
        Protocol::OBJECT_TYPE_STRETCHING_MESH_EFFECT,
        GAME->Current_Level(),
        TEXT("Layer_Effect"), 
        &desc
    );

    if (spawend)
    {
        spawend->Set_Owner(context.owner->GetSharedPtr<GameObject>());
        _spawnedEffect = spawend;
    }
}

void ANS_StretchingMesh::On_Tick(const FAnimNotifyContext& context)
{
     if (!context.owner || !context.model) return;

    auto effect = dynamic_pointer_cast<StretchingMeshEffect>(_spawnedEffect.lock());
    if (!effect) return;

    const Matrix* socketMatrix = context.model->Get_SocketBoneMatrixPtr(_boneName);
    if (socketMatrix)
    {
        Matrix boneWorldMatrix = (*socketMatrix) * context.owner->Get_Transform()->Get_WorldMatrix();
        Vec3 currentHandPos = boneWorldMatrix.Translation();
        
        effect->Update_TargetPosition(currentHandPos);
    }
}

void ANS_StretchingMesh::On_End(const FAnimNotifyContext& context)
{
    auto pEffect = _spawnedEffect.lock();
    if (pEffect)
    {
        pEffect->Set_Destroy(true);
    }
}

NS_END
