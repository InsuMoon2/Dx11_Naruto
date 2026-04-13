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
    PROPERTY_STRING_JSON("시작 뼈 이름",  "bone_name",   _boneName);
    PROPERTY_FLOAT_JSON("원본 길이",      "mesh_length", _originalLength, 0.1f, 10.f);
    PROPERTY_VEC3_JSON("굵기(두께)",      "thickness",   _thickness, 0.1f);

    return true;
}

void AN_StretchingMesh_Start::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner || !context.model)
        return;

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    if (!skillCom)
        return;

    const Matrix* socketMatrix = context.model->Get_SocketBoneMatrixPtr(_boneName);
    if (!socketMatrix) 
        return;

    Matrix boneWorldMatrix = (*socketMatrix) * context.owner->Get_Transform()->Get_WorldMatrix();
    Vec3 startWorldPos = boneWorldMatrix.Translation();

    StretchingMeshEffect::FStretchingMeshDesc desc{};
    desc.position           = startWorldPos;     // 고정 시작점
    desc.effectAssetName    = _effectAssetName;               
    desc.meshOriginalLength = _originalLength;
    desc.thickness          = _thickness;
    
    desc.ownerObj           = context.owner->GetSharedPtr<GameObject>();
    desc.trackBoneName      = _boneName; 

    auto spawned = GAME->Clone_And_Add_GameObject(
        0, 
        Protocol::OBJECT_TYPE_STRETCHING_MESH_EFFECT,
        GAME->Current_Level(),
        TEXT("Layer_Effect"), 
        &desc
    );

    if (spawned)
    {
        skillCom->Set_ActiveStretchingMesh(spawned);
    }
}

NS_END
