#include "pch.h"
#include "StaticMeshActor.h"
#include "GameInstance.h"
#include "Shader.h"
#include "Model.h"
#include "ModelMaterial.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(StaticMeshActor, Protocol::OBJECT_TYPE_STATIC_MESH)

static bool Is_KonohaDistanceCullEnabled()
{
    return GAME->Current_Level() == ETOI(ELevelType::Konoha);
}

static float Get_KonohaStaticMeshCullDistance()
{
    return 220.f;
}

// Gameplay는 산맥/배경 static caster가 shadow map을 덮지 않도록 Area 내부 주요 mesh만 shadow caster로 허용한다.
static bool Should_UseGameplayStaticShadowCaster(const StaticMeshActor& actor)
{
    const wstring& name = actor.Get_Name(); // Gameplay에서 큰 frustum band를 만들지 않는 소형 static caster만 선별하기 위한 이름이다.

    // Ground/wall/building처럼 큰 면은 shadow receiver로만 두고, Tree 계열처럼 작은 물체만 caster로 복구한다.
    return name.find(L"Tree") != wstring::npos;
}

// Konoha는 대형 merged 건물/지형이 많아서 플레이어 추적 shadow map에는 가까운 소형 caster만 넣는다.
static bool Should_UseKonohaStaticShadowCaster(const StaticMeshActor& actor)
{
    const wstring& name = actor.Get_Name(); // Konoha의 대형 배경 caster를 이름 기준으로 제외하기 위한 static mesh 이름이다.

    if (name.find(L"MERGED") != wstring::npos ||
        name.find(L"REDUCTION") != wstring::npos ||
        name.find(L"Distance") != wstring::npos ||
        name.find(L"Far") != wstring::npos ||
        name.find(L"Ground") != wstring::npos ||
        name.find(L"Wall") != wstring::npos ||
        name.find(L"Building") != wstring::npos ||
        name.find(L"FaceRock") != wstring::npos)
    {
        return false;
    }

    const bool isSmallCaster = // Konoha shadow map에 넣어도 화면을 덮지 않는 소형 prop 이름군이다.
        name.find(L"Tree") != wstring::npos ||
        name.find(L"Lantern") != wstring::npos ||
        name.find(L"Stall") != wstring::npos ||
        name.find(L"Flags") != wstring::npos ||
        name.find(L"Signs") != wstring::npos ||
        name.find(L"Manhole") != wstring::npos ||
        name.find(L"WaterTank") != wstring::npos ||
        name.find(L"WoodBoard") != wstring::npos ||
        name.find(L"SteelBoard") != wstring::npos ||
        name.find(L"Balloon") != wstring::npos ||
        name.find(L"Cat") != wstring::npos;

    if (!isSmallCaster)
        return false;

    auto transform = actor.Get_Transform();
    if (!transform)
        return true;

    Vec3 shadowFocus = Vec3::Zero; // Konoha shadow caster 거리 판정은 카메라가 아니라 현재 shadow target/player 주변을 기준으로 한다.
    if (const FLightDesc* shadowDesc = GAME->Get_PrimaryShadowLightDesc())
        shadowFocus = shadowDesc->shadowTarget;
    else if (const Vec4* camPos4 = GAME->Get_CamPosition())
        shadowFocus = Vec3(camPos4->x, camPos4->y, camPos4->z);

    const Vec3 actorPos = transform->Get_WorldPosition();
    const float shadowCasterDistance = 95.f; // 80x45 ortho 영역 주변에 들어갈 가능성이 높은 prop만 허용한다.

    return Vec3::DistanceSquared(shadowFocus, actorPos) <= shadowCasterDistance * shadowCasterDistance;
}

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

bool StaticMeshActor::Is_RenderBatchSortable() const
{
    return true;
}

uint64 StaticMeshActor::Get_RenderBatchPrimaryKey() const
{
    return static_cast<uint64>(Protocol::COMPONENT_TYPE_SHADER_STATIC_MESH);
}

uint64 StaticMeshActor::Get_RenderBatchSecondaryKey() const
{
    if (!_modelGuid.empty())
        return static_cast<uint64>(hash<string>{}(_modelGuid));

    if (!_resolvedPath.empty())
        return static_cast<uint64>(hash<string>{}(_resolvedPath));

    return 0ull;
}

HRESULT StaticMeshActor::Render_Shadow()
{
    if (!_modelCom || !_shaderCom)
    {
#ifdef _DEBUG
        LOG_WARN("[StaticMeshShadow] skipped. model={}, shader={}, name='{}', guid='{}'",
            _modelCom != nullptr,
            _shaderCom != nullptr,
            Utils::ToString(Get_Name()),
            Get_GUID());
#endif
        return S_FALSE;
    }

    CHECK_FAILED(Bind_ShadowShaderResources(), E_FAIL);

    size_t numMeshes = _modelCom->Get_NumMeshes();
    if (numMeshes == 0)
    {
#ifdef _DEBUG
        LOG_WARN("[StaticMeshShadow] skipped. numMeshes=0, name='{}', guid='{}'",
            Utils::ToString(Get_Name()),
            Get_GUID());
#endif
        return S_FALSE;
    }

    for (size_t i = 0; i < numMeshes; ++i)
    {
        uint32 matIdx = _modelCom->Get_MeshMaterialIndex(static_cast<uint32>(i));
        auto material = _modelCom->Get_Material(matIdx);

        int hasDiffuseTexture = 0;
        int hasBlendDiffuseTexture = 0;
        int hasMaskTexture = 0;

        if (material)
        {
            hasDiffuseTexture = (material->Get_TextureCount(EMaterialTextureSlot::BaseColor) > 0) ? 1 : 0;
            hasBlendDiffuseTexture = (material->Get_TextureCount(EMaterialTextureSlot::BlendBaseColor) > 0) ? 1 : 0;
            hasMaskTexture = (material->Get_TextureCount(EMaterialTextureSlot::Mask) > 0) ? 1 : 0;
        }

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasDiffuseTexture", &hasDiffuseTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasBlendDiffuseTexture", &hasBlendDiffuseTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasMaskTexture", &hasMaskTexture, sizeof(int)), E_FAIL);

        if (hasDiffuseTexture != 0)
        {
            CHECK_FAILED(_modelCom->Bind_Material(_shaderCom, "g_DiffuseTexture", static_cast<uint32>(i), EMaterialTextureSlot::BaseColor, 0), E_FAIL);
        }
        else
        {
            CHECK_FAILED(_shaderCom->Bind_SRV("g_DiffuseTexture", nullptr), E_FAIL);
        }

        CHECK_FAILED(_shaderCom->Begin_Pass(3), E_FAIL);
        CHECK_FAILED(_modelCom->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

HRESULT StaticMeshActor::Bind_ShadowShaderResources()
{
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(GAME->Bind_ShadowMatrices(_shaderCom, "g_ViewMatrix", "g_ProjMatrix"), E_FAIL);

    return S_OK;
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

    const int32 currentLevel = GAME->Current_Level();
    const bool useShadowCaster =
        (currentLevel == ETOI(ELevelType::GamePlay))
        ? Should_UseGameplayStaticShadowCaster(*this)
        : ((currentLevel == ETOI(ELevelType::Konoha))
            ? Should_UseKonohaStaticShadowCaster(*this)
            : true);

    if (useShadowCaster)
        GAME->Add_RenderGroup(ERenderGroup::ShadowStatic, GetSharedPtr());

    // 거리비례 짜르기
    if (Is_KonohaDistanceCullEnabled())
    {
        const Vec4* camPos4 = GAME->Get_CamPosition();
        if (camPos4 && _transformCom)
        {
            const Vec3 cameraPos = Vec3(camPos4->x, camPos4->y, camPos4->z);
            const Vec3 actorPos = _transformCom->Get_WorldPosition();

            const float cullDistance = Get_KonohaStaticMeshCullDistance();
            const float cullDistanceSq = cullDistance * cullDistance;

            if (Vec3::DistanceSquared(cameraPos, actorPos) > cullDistanceSq)
                return;
        }
    }

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
}

HRESULT StaticMeshActor::Render()
{
    GameObject::Render();

    if (!_modelCom || !_shaderCom)
        return S_OK;

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    size_t numMeshes = _modelCom->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; ++i)
    {
        uint32 matIdx = _modelCom->Get_MeshMaterialIndex(static_cast<uint32>(i));
        auto material = _modelCom->Get_Material(matIdx);

        Vec4 baseColorFactor = Vec4(1.f, 1.f, 1.f, 1.f);

        int hasDiffuseTexture = 0;
        int hasBlendDiffuseTexture = 0;
        int hasMaskTexture = 0;

        int baseColorUVChannel = 0;
        int blendDiffuseUVChannel = 0;
        int maskUVChannel = 0;

        float baseColorUVScale = 1.f;
        float blendDiffuseUVScale = 1.f;
        float maskUVScale = 1.f;

        float maskScale = 1.f;
        float maskThreshold = 1.f;

        if (material)
        {
            baseColorFactor = material->Get_BaseColorFactor();

            hasDiffuseTexture =
                (material->Get_TextureCount(EMaterialTextureSlot::BaseColor) > 0) ? 1 : 0;

            hasBlendDiffuseTexture =
                (material->Get_TextureCount(EMaterialTextureSlot::BlendBaseColor) > 0) ? 1 : 0;

            hasMaskTexture =
                (material->Get_TextureCount(EMaterialTextureSlot::Mask) > 0) ? 1 : 0;

            maskScale = material->Get_MaskScale();
            maskThreshold = material->Get_MaskThreshold();

            baseColorUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::BaseColor, 0));
            blendDiffuseUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::BlendBaseColor, 0));
            maskUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::Mask, 0));

            baseColorUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::BaseColor, 0);
            blendDiffuseUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::BlendBaseColor, 0);
            maskUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::Mask, 0);
        }

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColorFactor", &baseColorFactor, sizeof(Vec4)), E_FAIL);

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasDiffuseTexture", &hasDiffuseTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasBlendDiffuseTexture", &hasBlendDiffuseTexture, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasMaskTexture", &hasMaskTexture, sizeof(int)), E_FAIL);

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColorUVChannel", &baseColorUVChannel, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BlendDiffuseUVChannel", &blendDiffuseUVChannel, sizeof(int)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskUVChannel", &maskUVChannel, sizeof(int)), E_FAIL);

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BaseColorUVScale", &baseColorUVScale, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_BlendDiffuseUVScale", &blendDiffuseUVScale, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskUVScale", &maskUVScale, sizeof(float)), E_FAIL);

        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskScale", &maskScale, sizeof(float)), E_FAIL);
        CHECK_FAILED(_shaderCom->Bind_RawValue("g_MaskThreshold", &maskThreshold, sizeof(float)), E_FAIL);

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

    const uint32 staticLevelIndex = ETOI(ELevelType::Static);
    const uint32 modelKey = static_cast<uint32>(hash<string>{}(_modelGuid));

    // 같은 model_guid 프로토타입이 이미 Static 레벨에 있으면, 디스크에서 모델을 다시 만들지 않는다.
    if (GAME->Find_Component_Prototype(staticLevelIndex, modelKey) == nullptr)
    {
        const Matrix scaleMatrix = Matrix::CreateScale(1.f);
        const Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));
        const Matrix preTransform = scaleMatrix * rotationMatrix;

        auto proto = Model::Create(
            _device, _context, EMeshVertexType::StaticMesh, _resolvedPath, preTransform, true);
        CHECK_NULL(proto, E_FAIL);

        CHECK_FAILED(GAME->Add_Component_Prototype(staticLevelIndex, modelKey, proto), E_FAIL);
    }

    CHECK_FAILED(Add_Component(staticLevelIndex, modelKey, _modelCom), E_FAIL);
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
