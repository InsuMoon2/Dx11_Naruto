#include "pch.h"
#include "Weapon.h"
#include "Model.h"
#include "Shader.h"
#include "GameObject_Factory.h"
#include "Bounding_OBB.h"
#include "Collider.h"
#include "Character.h"
#include "PlayerStateMachine.h"
#include "PlayerState_Attack.h"
#include "CombatStat.h"
#include "MyPlayer.h"

REGISTER_GAMEOBJECT(Weapon, Protocol::OBJECT_TYPE_PART_WEAPON)

Weapon::Weapon(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : PartObject(device, context)
{
}

Weapon::Weapon(const Weapon& rhs)
    : PartObject(rhs)
    , _socketMatrix(rhs._socketMatrix)
{
}

HRESULT Weapon::Initialize_Prototype()
{
    return PartObject::Initialize_Prototype();
}

HRESULT Weapon::Initialize(void* arg)
{
    CHECK_FAILED(PartObject::Initialize(arg), E_FAIL);

    FWeaponDesc* desc = static_cast<FWeaponDesc*>(arg);
    wstring modelTag = L"";

    if (desc)
    {
        modelTag = desc->modelAssetTag;
        _socketMatrix = desc->socketMatrix;
    }

    CHECK_FAILED(Ready_Components(modelTag), E_FAIL);

    if (_transformCom)
    {
        _transformCom->Set_LocalScale(Vec3(0.01f, 0.01f, 0.01f));
        _transformCom->Set_LocalRotation(0.f, 180.f, 0.f);
    }

    Set_SwordTrailLocalPoints(
        Vec3(0.f, 0.f, 0.f),
        Vec3(0.f, 20000.f, 0.f));

    return S_OK;
}

void Weapon::Priority_Update(float timeDelta)
{
    PartObject::Priority_Update(timeDelta);
}

void Weapon::Update(float timeDelta)
{
    PartObject::Update(timeDelta);

    Matrix socketMatrix = Matrix::Identity;

    if (_socketMatrix)
    {
        socketMatrix = *_socketMatrix;

        // 부모 Scale 영향 제거
        Vec3 right = socketMatrix.Right();
        Vec3 up = socketMatrix.Up();
        Vec3 backward = socketMatrix.Backward();

        right.Normalize();
        up.Normalize();
        backward.Normalize();

        socketMatrix.Right(right);
        socketMatrix.Up(up);
        socketMatrix.Backward(backward);
    }

    Update_CombinedWorldMatrix(Get_Transform()->Get_WorldMatrix() * socketMatrix);
}

void Weapon::Late_Update(float timeDelta)
{
    PartObject::Late_Update(timeDelta);

    if (_collider)
    {
        _collider->Update_Collider(_combinedWorldMatrix);

        // 활성화됐을 때만 추가
        GAME->Add_Collider(_collider);
        
    }

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT Weapon::Render()
{
    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    if (!_model)
        return S_OK;

    size_t numMeshes = _model->Get_NumMeshes();

    
    // 툰 셰이딩
    {
        // 외곽선 색
        const Vec4 outlineColor = Vec4(0.04f, 0.05f, 0.08f, 1.f);

        // 외곽선 두께
        const float outlineThickness = 0.0035f;

        CHECK_FAILED(_shader->Bind_RawValue("g_OutlineColor", &outlineColor, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_shader->Bind_RawValue("g_OutlineThickness", &outlineThickness, sizeof(float)), E_FAIL);
    }


    for (size_t i = 0; i < numMeshes; i++)
    {
        _model->Bind_Material(_shader, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

        CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(i), E_FAIL);

        //CHECK_FAILED(_shader->Begin_Pass(1), E_FAIL);
        //CHECK_FAILED(_model->Render(i), E_FAIL);
    }

    return S_OK;
}

void Weapon::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    PartObject::OnBeginOverlap(self, other);

    Shared<Character> hitted = dynamic_pointer_cast<Character>(other->Get_Owner());
    if (!hitted)
        return;

    Shared<GameObject> owner = nullptr;
    if (auto parentTransform = _parentTransform.lock())
        owner = parentTransform->Get_Owner();
    if (!owner)
        return;

    if (hitted.get() == owner.get())
        return;

    auto combatStat = owner->Get_Component<CombatStat>();
    if (combatStat && combatStat->Apply_Damage(hitted.get())) 
    {
        if (auto myPlayer = dynamic_pointer_cast<MyPlayer>(owner))
            myPlayer->Add_ComboHit();
    }
}

void Weapon::Set_ColliderActive(bool active)
{
    if (_collider)
    {
        _collider->Set_IsActive(active);
    }
}

void Weapon::Set_SwordTrailLocalPoints(const Vec3& rootLocal, const Vec3& tipLocal)
{
    _swordTrailRootLocal = rootLocal;
    _swordTrailTipLocal = tipLocal;
}

bool Weapon::Get_SwordTrailWorldPoints(Vec3& outRootWorld, Vec3& outTipWorld) const
{
    outRootWorld = XMVector3TransformCoord(_swordTrailRootLocal, _combinedWorldMatrix);
    outTipWorld = XMVector3TransformCoord(_swordTrailTipLocal, _combinedWorldMatrix);

    return true;
}

HRESULT Weapon::Ready_Components(const wstring& modelAssetTag)
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXMESH, _shader), E_FAIL);

    if (!modelAssetTag.empty())
    {
        string tagStr = Utils::ToString(modelAssetTag);
        uint32 modelKey = static_cast<uint32>(std::hash<string>{}(tagStr));
        CHECK_FAILED(Add_Component(modelKey, _model), E_FAIL);
    }

    // 충돌체 추가
    Bounding_OBB::FBoundingOBBDesc obbDesc{};
    obbDesc.extents = Vec3(5.f, 30.f, 5.f);
    obbDesc.center = Vec3(0.f, obbDesc.extents.y, 0.f);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_OBB, _collider, &obbDesc), E_FAIL);
    _collider->Set_CollisionPreset(Collision_Preset::Player_Attack);
    _collider->Set_IsActive(false); // 기본 비활성 -> 공격 시 활성화되도록

    return S_OK;
}

HRESULT Weapon::Bind_ShaderResources()
{
    _shader->Bind_Matrix("g_WorldMatrix", &_combinedWorldMatrix);
    _shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    return S_OK;
}

Shared<GameObject> Weapon::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Weapon>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Weapon");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> Weapon::Clone(void* arg)
{
    auto clone = make_shared<Weapon>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Weapon");

        return nullptr;
    }

    return clone;
}

void Weapon::Free()
{
    PartObject::Free();
}
