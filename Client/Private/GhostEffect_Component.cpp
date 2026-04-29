#include "pch.h"
#include "GhostEffect_Component.h"
#include "Mesh.h"
#include "GameObject.h"

GhostEffect_Component::GhostEffect_Component(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

GhostEffect_Component::GhostEffect_Component(const GhostEffect_Component& rhs)
    : Component(rhs)
{
}

HRESULT GhostEffect_Component::Initialize(void* arg)
{
    _ghostShader = static_pointer_cast<Shader>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_SHADER_GHOST_EFFECT));

    CHECK_NULL(_ghostShader, E_FAIL);

    return S_OK;
}

void GhostEffect_Component::BeginPlay()
{
    Component::BeginPlay();

    auto owner = Get_Owner();
    if (owner)
    {
        _ownerTransform = owner->Get_Transform();
        _ownerModel = owner->Get_Component<Model>();
    }
}

void GhostEffect_Component::Update_GhostEffect(float timeDelta)
{
    for (auto iter = _ghostSnapshots.begin(); iter != _ghostSnapshots.end();)
    {
        iter->lifespan -= timeDelta;
        if (iter->lifespan <= 0)
        {
            iter = _ghostSnapshots.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    // 이팩트 생성 중이면 일정 주기마다 스냅샷
    if (_settings.active && !_ownerTransform.expired() && !_ownerModel.expired())
    {
        _settings.captureTimer += timeDelta;

        if (_settings.captureTimer >= _settings.captureInterval)
        {
            _settings.captureTimer = 0.f;

            auto transform = _ownerTransform.lock();
            auto model = _ownerModel.lock();

            FGhostSnapshot snapshot;
            snapshot.lifespan = _settings.defaultLifespan;
            snapshot.maxLifespan = _settings.defaultLifespan;
            snapshot.worldMatrix = transform->Get_WorldMatrix();

            snapshot.boneMatrices = model->Get_BoneMatrices();
            snapshot.color = _settings.color;
            snapshot.rimColor = _settings.rimColor;

            _ghostSnapshots.push_back(snapshot);
        }
    }

}

HRESULT GhostEffect_Component::Render()
{
    if (_ghostSnapshots.empty() || !_ghostShader || _ownerModel.expired())
        return S_OK;

    auto model = _ownerModel.lock();
    if (!model)
        return S_OK;

    CHECK_FAILED(_ghostShader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_ghostShader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);
    CHECK_FAILED(GAME->Bind_CamPosition(_ghostShader, "g_CamPosition"), E_FAIL);

    // 고스트 스냅샷이 셰이더의 g_BoneMatrices 배열 크기와 같은 개수로 바인딩되도록 맞춘다.
    constexpr uint32 MAX_GHOST_BONES = 768;
    vector<Matrix> paddedBoneMatrices;
    paddedBoneMatrices.resize(MAX_GHOST_BONES, Matrix::Identity);

    for (const auto& ghost : _ghostSnapshots)
    {
        CHECK_FAILED(_ghostShader->Bind_Matrix("g_WorldMatrix", &ghost.worldMatrix), E_FAIL);
        CHECK_FAILED(_ghostShader->Bind_RawValue("g_GhostColor", &ghost.color, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_ghostShader->Bind_RawValue("g_RimColor", &ghost.rimColor, sizeof(Vec4)), E_FAIL);

        float alpha = 0.f;
        if (ghost.maxLifespan > FLT_EPSILON)
            alpha = ghost.lifespan / ghost.maxLifespan;

        alpha = ::clamp(alpha, 0.f, 1.f);
        CHECK_FAILED(_ghostShader->Bind_RawValue("g_GhostAlpha", &alpha, sizeof(float)), E_FAIL);

        const size_t boneCount = min<size_t>(ghost.boneMatrices.size(), MAX_GHOST_BONES);
        for (size_t i = 0; i < boneCount; ++i)
        {
            paddedBoneMatrices[i] = ghost.boneMatrices[i];
        }

        CHECK_FAILED(
            _ghostShader->Bind_RawValue(
                "g_BoneMatrices",
                paddedBoneMatrices.data(),
                sizeof(Matrix) * MAX_GHOST_BONES),
            E_FAIL);

        CHECK_FAILED(_ghostShader->Begin_Pass(0), E_FAIL);

        const auto& meshes = model->Get_Meshes();
        for (uint32 meshIndex = 0; meshIndex < static_cast<uint32>(meshes.size()); ++meshIndex)
        {
            CHECK_FAILED(model->Render(meshIndex), E_FAIL);
        }

        for (size_t i = 0; i < boneCount; ++i)
        {
            paddedBoneMatrices[i] = Matrix::Identity;
        }
    }

    return S_OK;
}


void GhostEffect_Component::Start_GhostEffect(float interval, float lifespan, Vec4 color, Vec4 rimColor)
{
    _settings.active = true;
    _settings.captureTimer = interval;
    _settings.captureInterval = interval;
    _settings.defaultLifespan = lifespan;
    _settings.color = color;
    _settings.rimColor = rimColor;
}

void GhostEffect_Component::Stop_GhostEffect()
{
    _settings.active = false;
}

Shared<GhostEffect_Component> GhostEffect_Component::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<GhostEffect_Component>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : GhostEffect_Component");
        return nullptr;
    }

    return instance;
}

Shared<Component> GhostEffect_Component::Clone(void* arg)
{
    auto clone = make_shared<GhostEffect_Component>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : GhostEffect_Component");
        return nullptr;
    }

    return clone;
}
