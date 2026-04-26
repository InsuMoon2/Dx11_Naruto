#include "pch.h"
#include "SocketPartObject.h"

#include "GameObject_Factory.h"
#include "Model.h"
#include "Shader.h"

REGISTER_GAMEOBJECT(SocketPartObject, Protocol::OBJECT_TYPE_PART_SOCKET)

NS_BEGIN(Client)

SocketPartObject::SocketPartObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : PartObject(device, context)
{
}

SocketPartObject::SocketPartObject(const SocketPartObject& rhs)
    : PartObject(rhs)
    , _socketMatrix(rhs._socketMatrix)
    , _localOffset(rhs._localOffset)
    , _localRotation(rhs._localRotation)
    , _localScale(rhs._localScale)
{
}

HRESULT SocketPartObject::Initialize_Prototype()
{
    CHECK_FAILED(PartObject::Initialize_Prototype(), E_FAIL);
    return S_OK;
}

HRESULT SocketPartObject::Initialize(void* arg)
{
    CHECK_FAILED(PartObject::Initialize(arg), E_FAIL);

    FSocketPartDesc* desc = static_cast<FSocketPartDesc*>(arg);
    wstring modelTag = L"";

    if (desc)
    {
        modelTag = desc->modelAssetTag;
        _socketMatrix = desc->socketMatrix;
        _localOffset = desc->localOffset;
        _localRotation = desc->localRotation;
        _localScale = desc->localScale;
    }

    CHECK_FAILED(Ready_Components(modelTag), E_FAIL);

    return S_OK;
}

void SocketPartObject::Update(float timeDelta)
{
    PartObject::Update(timeDelta);

    Refresh_CombinedWorldMatrix();
}

void SocketPartObject::Late_Update(float timeDelta)
{
    PartObject::Late_Update(timeDelta);

    Refresh_CombinedWorldMatrix();

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
    GAME->Add_RenderGroup(ERenderGroup::ShadowDynamic, GetSharedPtr());
}

void SocketPartObject::Refresh_CombinedWorldMatrix()
{
    Matrix socketMatrix = Matrix::Identity;

    if (_socketMatrix)
    {
        socketMatrix = *_socketMatrix;

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

    Matrix localMatrix =
        Matrix::CreateScale(_localScale) *
        Matrix::CreateFromYawPitchRoll(
            XMConvertToRadians(_localRotation.y),
            XMConvertToRadians(_localRotation.x),
            XMConvertToRadians(_localRotation.z)) *
        Matrix::CreateTranslation(_localOffset) *
        Get_Transform()->Get_WorldMatrix();

    Update_CombinedWorldMatrix(localMatrix * socketMatrix);
}

HRESULT SocketPartObject::Render()
{
    if (!_model || !_shader)
        return S_OK;

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    const size_t numMeshes = _model->Get_NumMeshes();

    const Vec4 outlineColor = Vec4(0.04f, 0.05f, 0.08f, 1.f);
    const float outlineThickness = 0.0035f;

    CHECK_FAILED(_shader->Bind_RawValue("g_OutlineColor", &outlineColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_OutlineThickness", &outlineThickness, sizeof(float)), E_FAIL);

    for (size_t i = 0; i < numMeshes; ++i)
    {
        CHECK_FAILED(_model->Bind_Material(_shader, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0), E_FAIL);
        CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

HRESULT SocketPartObject::Render_Shadow()
{
    if (!_model || !_shader)
        return S_OK;

    CHECK_FAILED(Bind_ShadowShaderResources(), E_FAIL);

    const size_t numMeshes = _model->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; ++i)
    {
        CHECK_FAILED(_model->Bind_Material(_shader, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0), E_FAIL);
        CHECK_FAILED(_shader->Begin_Pass(2), E_FAIL);
        CHECK_FAILED(_model->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

HRESULT SocketPartObject::Ready_Components(const wstring& modelAssetTag)
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXMESH, _shader), E_FAIL);

    if (!modelAssetTag.empty())
    {
        string tagStr = Utils::ToString(modelAssetTag);
        uint32 modelKey = static_cast<uint32>(hash<string>{}(tagStr));
        CHECK_FAILED(Add_Component(modelKey, _model), E_FAIL);
    }

    return S_OK;
}

HRESULT SocketPartObject::Bind_ShaderResources()
{
    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &_combinedWorldMatrix), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    return S_OK;
}

HRESULT SocketPartObject::Bind_ShadowShaderResources()
{
    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &_combinedWorldMatrix), E_FAIL);
    CHECK_FAILED(GAME->Bind_ShadowMatrices(_shader, "g_ViewMatrix", "g_ProjMatrix"), E_FAIL);

    return S_OK;
}

Shared<GameObject> SocketPartObject::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SocketPartObject>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : SocketPartObject");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> SocketPartObject::Clone(void* arg)
{
    auto clone = make_shared<SocketPartObject>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : SocketPartObject");
        return nullptr;
    }

    return clone;
}

void SocketPartObject::Free()
{
    PartObject::Free();
}

NS_END
