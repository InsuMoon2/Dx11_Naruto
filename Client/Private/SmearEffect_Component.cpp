#include "pch.h"
#include "SmearEffect_Component.h"

#include "GameObject.h"
#include "Transform.h"
#include "Model.h"
#include "Shader.h"
#include "MovementComponent.h"
#include "ContainerObject.h"
#include "PartObject.h"

SmearEffect_Component::SmearEffect_Component(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

SmearEffect_Component::SmearEffect_Component(const SmearEffect_Component& rhs)
    : Component(rhs)
    , _settings(rhs._settings)
{
}

HRESULT SmearEffect_Component::Initialize(void* arg)
{
    _smearSkelShader = static_pointer_cast<Shader>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_SHADER_SMEAR_EFFECT));
    CHECK_NULL(_smearSkelShader, E_FAIL);

    return S_OK;
}

void SmearEffect_Component::BeginPlay()
{
    Component::BeginPlay();

    auto owner = Get_Owner();
    if (!owner)
        return;

    _ownerObject = owner;
    _ownerTransform = owner->Get_Transform();
    _ownerMovement = owner->Get_Component<MovementComponent>();
}

void SmearEffect_Component::Update_Smear(float timeDelta)
{
    for (auto iter = _snapshots.begin(); iter != _snapshots.end();)
    {
        iter->life -= timeDelta;

        if (iter->life <= 0.f)
            iter = _snapshots.erase(iter);
        else
            ++iter;
    }

    // 새 스냅샷 생성을 켜 둔 상태가 아니면 정리만
    if (_settings.active == false)
        return;

    _settings.captureTimer += timeDelta;

    // 일정 간격마다 현재 전신 스켈레탈 상태를 캡처
    while (_settings.captureTimer >= _settings.captureInterval)
    {
        _settings.captureTimer -= _settings.captureInterval;
        Capture_CurrentSnapshot();
    }
}

HRESULT SmearEffect_Component::Render()
{
    if (_snapshots.empty())
        return S_OK;

    // 오래된 스냅샷부터 순서대로 그려 자연스럽게 뒤에 깔리게
    for (const auto& snapshot : _snapshots)
    {
        CHECK_FAILED(Render_Snapshot(snapshot), E_FAIL);
    }

    return S_OK;
}

void SmearEffect_Component::Start_Smear(
    float captureInterval,
    float lifespan,
    float smearLength,
    Vec4 baseColor,
    Vec4 edgeColor)
{
    _settings.active = true;
    _settings.captureTimer = captureInterval;
    _settings.captureInterval = max(0.001f, captureInterval);
    _settings.lifespan = max(0.01f, lifespan);
    _settings.smearLength = max(0.01f, smearLength);
    _settings.baseColor = baseColor;
    _settings.edgeColor = edgeColor;
}

void SmearEffect_Component::Stop_Smear()
{
    _settings.active = false;
    _settings.captureTimer = 0.f;
}

void SmearEffect_Component::Capture_CurrentSnapshot()
{
    auto owner = _ownerObject.lock();
    auto ownerTransform = _ownerTransform.lock();
    if (!owner || !ownerTransform)
        return;

    FSmearSnapshot snapshot{};
    snapshot.life = _settings.lifespan;
    snapshot.maxLife = _settings.lifespan;
    snapshot.smearDir = Resolve_SmearDirection();

    if (auto ownerModel = owner->Get_Component<Model>())
    {
        if (ownerModel->Get_ModelType() == EMeshVertexType::SkeletalMesh)
        {
            FRenderableSnapshot renderable{};
            renderable.model = ownerModel;
            renderable.worldMatrix = ownerTransform->Get_WorldMatrix();
            renderable.modelType = ownerModel->Get_ModelType();
            renderable.boneMatrices = ownerModel->Get_BoneMatrices();

            snapshot.renderables.push_back(std::move(renderable));
        }
    }

    auto container = dynamic_pointer_cast<ContainerObject>(owner);
    if (container)
    {
        for (uint32 i = 0; i < ETOI(ContainerObject::EPartSlot::END); ++i)
        {
            const auto slot = static_cast<ContainerObject::EPartSlot>(i);
            auto part = container->Get_PartObject(slot);
            if (!part)
                continue;

            auto partModel = part->Get_Component<Model>();
            if (!partModel)
                continue;

            if (partModel->Get_ModelType() != EMeshVertexType::SkeletalMesh)
                continue;

            FRenderableSnapshot renderable{};
            renderable.model = partModel;
            renderable.worldMatrix = part->Get_CombinedWorldMatrix();
            renderable.modelType = partModel->Get_ModelType();
            renderable.boneMatrices = partModel->Get_BoneMatrices();

            snapshot.renderables.push_back(std::move(renderable));
        }
    }

    if (!snapshot.renderables.empty())
    {
        _snapshots.push_back(std::move(snapshot));
    }
}

Vec3 SmearEffect_Component::Resolve_SmearDirection() const
{
    auto movement = _ownerMovement.lock();
    if (movement)
    {
        Vec3 velocity = movement->Get_Velocity();
        velocity.y = 0.f;

        if (velocity.LengthSquared() > FLT_EPSILON)
        {
            velocity.Normalize();
            return velocity;
        }
    }

    auto transform = _ownerTransform.lock();
    if (transform)
    {
        Vec3 forward = transform->Get_WorldForward();
        forward.y = 0.f;

        if (forward.LengthSquared() > FLT_EPSILON)
        {
            forward.Normalize();
            return forward;
        }
    }

    return Vec3::Forward;
}

HRESULT SmearEffect_Component::Render_Snapshot(const FSmearSnapshot& snapshot)
{
    if (snapshot.maxLife <= FLT_EPSILON)
        return S_OK;

    const float ageRatio = 1.f - (snapshot.life / snapshot.maxLife);
    const float clampedAgeRatio = ::clamp(ageRatio, 0.f, 1.f);

    Vec3 smearDir = snapshot.smearDir;
    if (smearDir.LengthSquared() <= FLT_EPSILON)
        smearDir = Vec3::Forward;
    smearDir.Normalize();

    const Vec3 offset = -smearDir * (_settings.smearLength * clampedAgeRatio);

    // 스미어 스냅샷이 셰이더의 g_BoneMatrices 배열 크기와 같은 개수로 바인딩되도록 맞춘다.
    constexpr uint32 MAX_SMEAR_BONES = 768;

    for (const auto& renderable : snapshot.renderables)
    {
        if (!renderable.model)
            continue;

        Matrix worldMatrix = renderable.worldMatrix;
        worldMatrix.Translation(worldMatrix.Translation() + offset);

        CHECK_FAILED(_smearSkelShader->Bind_Matrix("g_WorldMatrix", &worldMatrix), E_FAIL);
        CHECK_FAILED(_smearSkelShader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
        CHECK_FAILED(_smearSkelShader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);
        CHECK_FAILED(GAME->Bind_CamPosition(_smearSkelShader, "g_CamPosition"), E_FAIL);

        CHECK_FAILED(_smearSkelShader->Bind_RawValue("g_SmearBaseColor", &_settings.baseColor, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_smearSkelShader->Bind_RawValue("g_SmearEdgeColor", &_settings.edgeColor, sizeof(Vec4)), E_FAIL);
        CHECK_FAILED(_smearSkelShader->Bind_RawValue("g_SmearAgeRatio", &clampedAgeRatio, sizeof(float)), E_FAIL);

        vector<Matrix> paddedBones(MAX_SMEAR_BONES, Matrix::Identity);

        const size_t boneCount = min<size_t>(renderable.boneMatrices.size(), MAX_SMEAR_BONES);
        for (size_t i = 0; i < boneCount; ++i)
        {
            paddedBones[i] = renderable.boneMatrices[i];
        }

        CHECK_FAILED(
            _smearSkelShader->Bind_RawValue(
                "g_BoneMatrices",
                paddedBones.data(),
                sizeof(Matrix) * MAX_SMEAR_BONES),
            E_FAIL);

        const uint32 meshCount = static_cast<uint32>(renderable.model->Get_NumMeshes());
        for (uint32 meshIndex = 0; meshIndex < meshCount; ++meshIndex)
        {
            CHECK_FAILED(_smearSkelShader->Begin_Pass(0), E_FAIL);
            CHECK_FAILED(renderable.model->Render(meshIndex), E_FAIL);
        }
    }

    return S_OK;
}

Shared<SmearEffect_Component> SmearEffect_Component::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SmearEffect_Component>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : SmearEffect_Component");
        return nullptr;
    }

    return instance;
}

Shared<Component> SmearEffect_Component::Clone(void* arg)
{
    auto clone = make_shared<SmearEffect_Component>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : SmearEffect_Component");
        return nullptr;
    }

    return clone;
}

void SmearEffect_Component::Free()
{
    Component::Free();
}
