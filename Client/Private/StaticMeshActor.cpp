#include "pch.h"
#include "StaticMeshActor.h"
#include "GameInstance.h"
#include "Shader.h"
#include "Model.h"
#include "ModelMaterial.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(StaticMeshActor, Protocol::OBJECT_TYPE_STATIC_MESH)

StaticMeshActor::StaticMeshActor(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
    
}

StaticMeshActor::StaticMeshActor(const StaticMeshActor& rhs)
    : GameObject(rhs)
    , _modelGuid(rhs._modelGuid)
    , _resolvedPath(rhs._resolvedPath)
    , _shaderCom(rhs._shaderCom)
    , _modelCom(rhs._modelCom)
    , _isOutlineEnabled(rhs._isOutlineEnabled)
    , _outlineColor(rhs._outlineColor)
    , _outlineThickness(rhs._outlineThickness)
{
}

StaticMeshActor::~StaticMeshActor()
{
}

HRESULT StaticMeshActor::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT StaticMeshActor::Initialize(void* arg)
{
    FStaticMeshDesc* desc = static_cast<FStaticMeshDesc*>(arg);

    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    // desc가 있으면 기존처럼 즉시 모델 준비
    if (desc && !desc->modelGuid.empty())
    {
        CHECK_FAILED(Apply_ModelGuid(desc->modelGuid), E_FAIL);
        CHECK_FAILED(Ensure_ModelReady(), E_FAIL);
    }

    return S_OK;
}

void StaticMeshActor::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void StaticMeshActor::Update(float timeDelta)
{
    GameObject::Update(timeDelta);
}

void StaticMeshActor::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
}

HRESULT StaticMeshActor::Render()
{
    GameObject::Render();

    if (!_modelCom || !_shaderCom)
        return S_OK;

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    int isOutlineEnabled = _isOutlineEnabled ? 1 : 0;
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_IsOutlineEnabled", &isOutlineEnabled, sizeof(int)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_OutlineColor", &_outlineColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_OutlineThickness", &_outlineThickness, sizeof(float)), E_FAIL);

    size_t numMeshes = _modelCom->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; ++i)
    {
        uint32 matIdx = _modelCom->Get_MeshMaterialIndex(static_cast<uint32>(i));
        auto material = _modelCom->Get_Material(matIdx);

        Vec4 baseColorFactor = Vec4(1.f, 1.f, 1.f, 1.f);
        Vec4 shadowColor = Vec4(1.f, 1.f, 1.f, 1.f);

        int hasDiffuseTexture = 0;
        int hasBlendDiffuseTexture = 0;
        int hasMaskTexture = 0;
        int hasBlendNormalTexture = 0;
        int hasUnevenColorTexture = 0;

        // [추가] slot별 UV 채널 인덱스를 shader로 넘겨 올바른 UV 세트를 선택하게 한다.
        int baseColorUVChannel = 0;
        int blendDiffuseUVChannel = 0;
        int maskUVChannel = 0;
        int blendNormalUVChannel = 0;
        int unevenColorUVChannel = 0;

        // [추가] slot별 UV 타일링 스케일을 shader로 넘겨 원본 material의 샘플링 비율을 복원한다.
        float baseColorUVScale = 1.f;
        float blendDiffuseUVScale = 1.f;
        float maskUVScale = 1.f;
        float blendNormalUVScale = 1.f;
        float unevenColorUVScale = 1.f;

        float maskScale = 1.f;
        float maskThreshold = 1.f;

        float blendNormalStrength = 1.f;
        float unevenColorScale = 1.f;

        if (material)
        {
            baseColorFactor = material->Get_BaseColorFactor();
            shadowColor = material->Get_ShadowColor();

            hasDiffuseTexture =
                (material->Get_TextureCount(EMaterialTextureSlot::BaseColor) > 0) ? 1 : 0;

            hasBlendDiffuseTexture =
                (material->Get_TextureCount(EMaterialTextureSlot::BlendBaseColor) > 0) ? 1 : 0;

            hasMaskTexture =
                (material->Get_TextureCount(EMaterialTextureSlot::Mask) > 0) ? 1 : 0;

            hasBlendNormalTexture =
                (material->Get_TextureCount(EMaterialTextureSlot::BlendNormal) > 0) ? 1 : 0;

            hasUnevenColorTexture =
                (material->Get_TextureCount(EMaterialTextureSlot::UnevenColor) > 0) ? 1 : 0;

            maskScale = material->Get_MaskScale();
            maskThreshold = material->Get_MaskThreshold();
            blendNormalStrength = material->Get_BlendNormalStrength();
            unevenColorScale = material->Get_UnevenColorScale();

            baseColorUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::BaseColor, 0));
            blendDiffuseUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::BlendBaseColor, 0));
            maskUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::Mask, 0));
            blendNormalUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::BlendNormal, 0));
            unevenColorUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::UnevenColor, 0));

            baseColorUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::BaseColor, 0);
            blendDiffuseUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::BlendBaseColor, 0);
            maskUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::Mask, 0);
            blendNormalUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::BlendNormal, 0);
            unevenColorUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::UnevenColor, 0);
        }

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColorFactor", &baseColorFactor, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_ShadowColor", &shadowColor, sizeof(Vec4)), E_FAIL);

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasDiffuseTexture", &hasDiffuseTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasBlendDiffuseTexture", &hasBlendDiffuseTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasMaskTexture", &hasMaskTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasBlendNormalTexture", &hasBlendNormalTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasUnevenColorTexture", &hasUnevenColorTexture, sizeof(int)), E_FAIL);

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColorUVChannel", &baseColorUVChannel, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BlendDiffuseUVChannel", &blendDiffuseUVChannel, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskUVChannel", &maskUVChannel, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BlendNormalUVChannel", &blendNormalUVChannel, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_UnevenColorUVChannel", &unevenColorUVChannel, sizeof(int)), E_FAIL);

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColorUVScale", &baseColorUVScale, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BlendDiffuseUVScale", &blendDiffuseUVScale, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskUVScale", &maskUVScale, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BlendNormalUVScale", &blendNormalUVScale, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_UnevenColorUVScale", &unevenColorUVScale, sizeof(float)), E_FAIL);

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskScale", &maskScale, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskThreshold", &maskThreshold, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BlendNormalStrength", &blendNormalStrength, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_UnevenColorScale", &unevenColorScale, sizeof(float)), E_FAIL);

        if (hasDiffuseTexture != 0)
        {
            CHECK_FAILED(
                _modelCom->Bind_Material(_shaderCom, "g_DiffuseTexture",
                    static_cast<uint32>(i), EMaterialTextureSlot::BaseColor, 0), E_FAIL);
        }
        else
        {
            CHECK_FAILED(_shaderCom->Bind_SRV("g_DiffuseTexture", nullptr), E_FAIL);
        }

        if (hasBlendDiffuseTexture != 0)
        {
            CHECK_FAILED(
                _modelCom->Bind_Material(_shaderCom, "g_BlendDiffuseTexture",
                    static_cast<uint32>(i), EMaterialTextureSlot::BlendBaseColor, 0), E_FAIL);
        }
        else
        {
            CHECK_FAILED(_shaderCom->Bind_SRV("g_BlendDiffuseTexture", nullptr), E_FAIL);
        }

        if (hasMaskTexture != 0)
        {
            CHECK_FAILED(
                _modelCom->Bind_Material(_shaderCom, "g_MaskTexture",
                    static_cast<uint32>(i), EMaterialTextureSlot::Mask, 0), E_FAIL);
        }
        else
        {
            CHECK_FAILED(_shaderCom->Bind_SRV("g_MaskTexture", nullptr), E_FAIL);
        }

        if (hasBlendNormalTexture != 0)
        {
            CHECK_FAILED(
                _modelCom->Bind_Material(_shaderCom, "g_BlendNormalTexture",
                    static_cast<uint32>(i), EMaterialTextureSlot::BlendNormal, 0), E_FAIL);
        }
        else
        {
            CHECK_FAILED(_shaderCom->Bind_SRV("g_BlendNormalTexture", nullptr), E_FAIL);
        }

        if (hasUnevenColorTexture != 0)
        {
            CHECK_FAILED(
                _modelCom->Bind_Material(_shaderCom, "g_UnevenColorTexture",
                    static_cast<uint32>(i), EMaterialTextureSlot::UnevenColor, 0), E_FAIL);
        }
        else
        {
            CHECK_FAILED(_shaderCom->Bind_SRV("g_UnevenColorTexture", nullptr), E_FAIL);
        }

        CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_modelCom->Render(static_cast<uint32>(i)), E_FAIL);
    }
    return S_OK;
}

HRESULT StaticMeshActor::Bind_ShaderResources()
{
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    return S_OK;
}

json StaticMeshActor::To_Json() const
{
    json j = GameObject::To_Json();
    j["model_guid"] = _modelGuid;

    return j;
}

void StaticMeshActor::From_Json(const json& data)
{
    GameObject::From_Json(data);

    if (data.contains("model_guid"))
    {
        string newGuid = data["model_guid"].get<string>();

        if (_modelGuid.empty())
        {
            if (FAILED(Apply_ModelGuid(newGuid)))
            {
                LOG_ERROR("StaticMeshActor::From_Json - model_guid 적용 실패: {}", newGuid);
                return;
            }
        }
        else if (_modelGuid != newGuid)
        {
            LOG_WARN("StaticMeshActor '{}' already has model_guid '{}', incoming guid '{}' ignored.",
                Utils::ToString(_name), _modelGuid, newGuid);
        }
    }

    if (!_modelGuid.empty())
    {
        if (FAILED(Ensure_ModelReady()))
        {
            LOG_ERROR("StaticMeshActor::From_Json - 모델 준비 실패: {}", _modelGuid);
        }
    }
        
}

HRESULT StaticMeshActor::Apply_ModelGuid(const string& modelGuid)
{
    if (modelGuid.empty())
    {
        LOG_ERROR("StaticMeshActor::Apply_ModelGuid - empty model guid");
        return E_FAIL;
    }

    if (!_modelGuid.empty() && _modelGuid != modelGuid)
    {
        LOG_WARN("StaticMeshActor '{}' already initialized with another guid. current='{}', incoming='{}'",
            Utils::ToString(_name), _modelGuid, modelGuid);
        return E_FAIL;
    }

    _modelGuid = modelGuid;

    return Resolve_ModelAsset();
}

HRESULT StaticMeshActor::Resolve_ModelAsset()
{
    if (_modelGuid.empty())
        return E_FAIL;

    _resolvedPath = Utils::ToString(GAME->Resolve_AssetPath(_modelGuid));

    if (_resolvedPath.empty())
    {
        LOG_ERROR("StaticMeshActor: GUID {} 에 해당하는 에셋을 찾을 수 없습니다.", _modelGuid);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT StaticMeshActor::Ensure_ModelReady()
{
    if (_modelCom && _shaderCom)
        return S_OK;

    CHECK_FAILED(Resolve_ModelAsset(), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

HRESULT StaticMeshActor::Ready_Components()
{
    if (_modelCom && _shaderCom)
        return S_OK;

    if (_modelGuid.empty())
    {
        LOG_ERROR("StaticMeshActor::Ready_Components - model guid is empty");
        return E_FAIL;
    }

    if (_resolvedPath.empty())
    {
        CHECK_FAILED(Resolve_ModelAsset(), E_FAIL);
    }

    uint32 modelKey = static_cast<uint32>(hash<string>{}(_modelGuid));

    Matrix scaleMatrix = Matrix::CreateScale(1.f);
    Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));
    Matrix preTransform = scaleMatrix * rotationMatrix;

    auto proto = Model::Create(_device, _context, EMeshVertexType::StaticMesh, _resolvedPath, preTransform, true);
    CHECK_NULL(proto, E_FAIL);

    GAME->Add_Component_Prototype(ETOI(ELevelType::Static), modelKey, proto);
    CHECK_FAILED(Add_Component(ETOI(ELevelType::Static), modelKey, _modelCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_STATIC_MESH, _shaderCom), E_FAIL);

    return S_OK;
}

Shared<GameObject> StaticMeshActor::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<StaticMeshActor>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : StaticMeshActor");
        return nullptr;

    }
    return instance;
}

Shared<GameObject> StaticMeshActor::Clone(void* arg)
{
    auto instance = make_shared<StaticMeshActor>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : StaticMeshActor");
        return nullptr;
    }

    return instance;
}

void StaticMeshActor::Free()
{
    GameObject::Free();
}
