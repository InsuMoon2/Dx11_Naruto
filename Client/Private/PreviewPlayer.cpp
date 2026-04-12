#include "pch.h"
#include "PreviewPlayer.h"
#include "GameObject_Factory.h"
#include "Model.h"
#include "PartObject.h"

REGISTER_GAMEOBJECT(PreviewPlayer, Protocol::OBJECT_TYPE_PREVIEW_PLAYER)

PreviewPlayer::PreviewPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Player(device, context)
{
}

PreviewPlayer::PreviewPlayer(const PreviewPlayer& rhs)
    : Player(rhs)
{
}

PreviewPlayer::~PreviewPlayer()
{
}

HRESULT PreviewPlayer::Initialize_Prototype()
{
    return Player::Initialize_Prototype();
}

HRESULT PreviewPlayer::Initialize(void* arg)
{
    CHECK_FAILED(Player::Initialize(arg), E_FAIL);

  
    return S_OK;
}

void PreviewPlayer::BeginPlay()
{
    Player::BeginPlay();

    if (_model && _model->Get_AnimationCount() > 0)
    {
        _model->Set_Animation(_model->Get_AnimationName(0), true);
    }

}

void PreviewPlayer::Priority_Update(float timeDelta)
{
    Player::Priority_Update(timeDelta);
}

void PreviewPlayer::Update(float timeDelta)
{
    Player::Update(timeDelta);
}

void PreviewPlayer::Late_Update(float timeDelta)
{
    Player::Late_Update(timeDelta);
}

HRESULT PreviewPlayer::Render()
{
    return Player::Render();
}

HRESULT PreviewPlayer::Bind_ShaderResources()
{
    return Player::Bind_ShaderResources();
}

HRESULT PreviewPlayer::Bind_Lights()
{
    return Player::Bind_Lights();
}

HRESULT PreviewPlayer::Ready_PartObjects()
{
    // Headgear
    PartObject::FPartObjectDesc headDesc{};
    //headDesc.parentMatrix = &_transformCom->Get_WorldMatrix();
    headDesc.parentTransform = _transformCom;
    headDesc.modelAssetTag = TEXT("Model_SnowHead");
    headDesc.masterPoseModel = _model;
    CHECK_FAILED(Add_PartObject(EPartSlot::Headegear, Protocol::OBJECT_TYPE_PART_OBJECT, &headDesc), E_FAIL);

    // Face
    PartObject::FPartObjectDesc faceDesc{};
    //faceDesc.parentMatrix = &_transformCom->Get_WorldMatrix();
    faceDesc.parentTransform = _transformCom;
    faceDesc.modelAssetTag = TEXT("Model_Face_Face1");
    faceDesc.masterPoseModel = _model;
    CHECK_FAILED(Add_PartObject(EPartSlot::Face, Protocol::OBJECT_TYPE_PART_OBJECT, &faceDesc), E_FAIL);

    // Onepiece
    PartObject::FPartObjectDesc onePieceDesc{};
    //onePieceDesc.parentMatrix = &_transformCom->Get_WorldMatrix();
    onePieceDesc.parentTransform = _transformCom;
    onePieceDesc.modelAssetTag = TEXT("Model_OnePiece_Armor3");
    onePieceDesc.masterPoseModel = _model;
    CHECK_FAILED(Add_PartObject(EPartSlot::Onepiece, Protocol::OBJECT_TYPE_PART_OBJECT, &onePieceDesc), E_FAIL);

}

Shared<GameObject> PreviewPlayer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<PreviewPlayer>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : PreviewPlayer");

        return nullptr;
    }

    return instance;
}

Shared<GameObject> PreviewPlayer::Clone(void* arg)
{
    auto clone = make_shared<PreviewPlayer>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : PreviewPlayer");

        return nullptr;
    }

    return clone;
}

void PreviewPlayer::Free()
{
    Player::Free();
}
