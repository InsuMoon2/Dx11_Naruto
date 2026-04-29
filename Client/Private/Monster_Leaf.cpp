#include "pch.h"
#include "Monster_Leaf.h"

#include "CharkraMove_Component.h"
#include "CombatStat.h"
#include "GameObject_Factory.h"
#include "PartObject.h"
#include "Model.h"
#include "SocketPartObject.h"

REGISTER_GAMEOBJECT(Monster_Leaf, Protocol::OBJECT_TYPE_MONSTER_LEAF)

IMPLEMENT_REFLECTION(Monster_Leaf)

bool Monster_Leaf::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "Monster_Leaf";

    return true;
}

Monster_Leaf::Monster_Leaf(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Monster(device, context)
{
}

Monster_Leaf::Monster_Leaf(const Monster_Leaf& rhs)
    : Monster(rhs)
{
}

HRESULT Monster_Leaf::Initialize_Prototype()
{
    CHECK_FAILED(Monster::Initialize_Prototype(), E_FAIL);

    return S_OK;
}

HRESULT Monster_Leaf::Initialize(void* arg)
{
    CHECK_FAILED(Monster::Initialize(arg), E_FAIL);

    // 글러브 준비 플래그를 초기화한다.
    // (BeginPlay에서 단 한 번만 Ready_GloveParts를 호출하기 위해 여기서 리셋)
    _glovePartsReady = false;
    _glovePartsClearedOnDeath = false;

    return S_OK;
}

void Monster_Leaf::BeginPlay()
{
    Monster::BeginPlay();

    // BeginPlay 시점에는 모델이 이미 초기화되어 있으므로 여기서 글러브를 한 번만 부착한다.
    // Update()에서 지연 부착하면 Edit→Play 반복 시 SocketPartObject의
    // Get_Transform()->Get_WorldMatrix()에 에디터 잔류 스케일이 남아 글러브가 거대해진다.
    if (!_glovePartsReady && _model)
    {
        if (SUCCEEDED(Ready_GloveParts()))
            _glovePartsReady = true;
    }
}

void Monster_Leaf::Update(float timeDelta)
{
    Monster::Update(timeDelta);

    if (!_glovePartsClearedOnDeath && _combatStat && _combatStat->Is_Dead())
        Clear_GloveParts_OnDeath();

    if (_chakraTrail)
        _chakraTrail->Update_ChakraMove(timeDelta);
}

void Monster_Leaf::Late_Update(float timeDelta)
{
    Monster::Late_Update(timeDelta);

}

HRESULT Monster_Leaf::Render()
{
     CHECK_FAILED(Monster::Render(), E_FAIL);

    if (_chakraTrail)
        CHECK_FAILED(_chakraTrail->Render(), E_FAIL);

    return S_OK;
}

HRESULT Monster_Leaf::Ready_Components()
{
    CHECK_FAILED(Monster::Ready_Components(), E_FAIL);

    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_CHAKRA_MOVE, _chakraTrail), E_FAIL);

    if (_chakraTrail)
    {
        _chakraTrail->Set_TrailTintColor(Vec4(0.18f, 1.00f, 0.35f, 1.f));
        _chakraTrail->Set_TrailEmissiveStrength(1.8f);
    }

    return S_OK;
}

Protocol::OBJECT_TYPE Monster_Leaf::Get_EnemyObjectType() const
{
    return Protocol::OBJECT_TYPE_MONSTER_LEAF;
}

HRESULT Monster_Leaf::Ready_GloveParts()
{
    // 왼손
    {
        SocketPartObject::FSocketPartDesc leftGloveDesc{};
        leftGloveDesc.parentTransform = _transformCom;
        leftGloveDesc.modelAssetTag = TEXT("Model_BoxingGlove_Left");
        leftGloveDesc.socketMatrix = _model
            ? _model->Get_SocketBoneMatrixPtr("LeftHand")
            : nullptr;
        leftGloveDesc.localScale = Vec3(0.01f, 0.01f, 0.01f);
        leftGloveDesc.localRotation = Vec3(0.f, 0.f, 0.f);
        leftGloveDesc.localOffset = Vec3::Zero;

        CHECK_FAILED(Add_PartObject(EPartSlot::Accessory, Protocol::OBJECT_TYPE_PART_SOCKET, &leftGloveDesc), E_FAIL);
    }
    // 오른손
    {
        SocketPartObject::FSocketPartDesc rightGloveDesc{};
        rightGloveDesc.parentTransform = _transformCom;
        rightGloveDesc.modelAssetTag = TEXT("Model_BoxingGlove_Right");
        rightGloveDesc.socketMatrix = _model
            ? _model->Get_SocketBoneMatrixPtr("RightHand")
            : nullptr;
        rightGloveDesc.localScale = Vec3(0.01f, 0.01f, 0.01f);
        rightGloveDesc.localRotation = Vec3(0.f, 0.f, 0.f);
        rightGloveDesc.localOffset = Vec3::Zero;

        CHECK_FAILED(Add_PartObject(EPartSlot::Weapon, Protocol::OBJECT_TYPE_PART_SOCKET, &rightGloveDesc), E_FAIL);
    }

    return S_OK;
}

void Monster_Leaf::Clear_GloveParts_OnDeath()
{
    _glovePartsClearedOnDeath = true;

    auto leftGlove = Get_PartObject(EPartSlot::Accessory);
    if (leftGlove)
        leftGlove->Set_Destroy(true);

    auto rightGlove = Get_PartObject(EPartSlot::Weapon);
    if (rightGlove)
        rightGlove->Set_Destroy(true);
}

Shared<Monster_Leaf> Monster_Leaf::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Monster_Leaf>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Monster_Leaf");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Monster_Leaf::Clone(void* arg)
{
    auto clone = make_shared<Monster_Leaf>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : Monster_Leaf");
        return nullptr;
    }

    return clone;
}

void Monster_Leaf::Free()
{
    Monster::Free();
}
