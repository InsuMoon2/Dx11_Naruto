#include "pch.h"
#include "CharkraMove_Component.h"

#include "AttachedEffectObject.h"
#include "GameObject.h"
#include "Model.h"
#include "Transform.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Trail.h"

CharkraMove_Component::CharkraMove_Component(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

CharkraMove_Component::CharkraMove_Component(const CharkraMove_Component& rhs)
    : Component(rhs)
{
}

HRESULT CharkraMove_Component::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CharkraMove_Component::Initialize(void* arg)
{
    _shader = static_pointer_cast<Shader>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_SHADER_CHAKRA));
    CHECK_NULL(_shader, E_FAIL);

    _texture = static_pointer_cast<Texture>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_TEXTURE_TRAIL));
    CHECK_NULL(_texture, E_FAIL);

    return S_OK;
}

void CharkraMove_Component::BeginPlay()
{
    Component::BeginPlay();

    if (!_hasStartedDefaultChakraMove)
        Start_DefaultChakraMove();
}

void CharkraMove_Component::Start_ChakraMove(const vector<FTrailChannelDesc>& channelDescs, float lifespan,
    float minOwnerMoveSpeed)
{
    _lifespan = lifespan;
    _minOwnerMoveSpeed = minOwnerMoveSpeed;
    _isEmitting = true;
    _hasPrevOwnerWorldPos = false;

    _channels.clear();
    _channels.reserve(channelDescs.size());

    for (const auto& channelDesc : channelDescs)
    {
        if (channelDesc.boneName.empty())
            continue;

        FTrailChannelData channel{};
        channel.boneName = channelDesc.boneName;
        channel.width = channelDesc.width;
        channel.baseLocalOffset = channelDesc.baseLocalOffset;
        channel.jitterRadius = channelDesc.jitterRadius;

        channel.lines.resize(max<uint32>(1, channelDesc.lineCount));

        for (uint32 i = 0; i < channel.lines.size(); ++i)
        {
            auto& line = channel.lines[i];

            line.viBuffer = static_pointer_cast<VIBuffer_Trail>(
                GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_VIBUFFER_TRAIL));

            line.localOffset = channel.baseLocalOffset;
            if (i != 0)
                line.localOffset += Pick_RandomLocalOffset(channel.jitterRadius);

            line.targetLocalOffset = line.localOffset;
            line.retargetTimer = 0.f;
            line.points.clear();
        }

        _channels.push_back(channel);
    }
}

void CharkraMove_Component::Stop_ChakraMove()
{
    _isEmitting = false;

    auto leftGlow = _leftGlowObject.lock();
    if (leftGlow && !leftGlow->Is_Destroy())
        leftGlow->Stop_AttachedEffect();

    auto rightGlow = _rightGlowObject.lock();
    if (rightGlow && !rightGlow->Is_Destroy())
        rightGlow->Stop_AttachedEffect();

    _leftGlowObject.reset();
    _rightGlowObject.reset();
}

bool CharkraMove_Component::Has_ActiveChakraMove() const
{
    if (_isEmitting)
        return true;

    for (const auto& channel : _channels)
    {
        for (const auto& line : channel.lines)
        {
            if (!line.points.empty())
                return true;
        }
    }

    return false;
}

void CharkraMove_Component::Update_ChakraMove(float timeDelta)
{
    //Cleanup_GlowObjects();
    //
    //if (_hasStartedDefaultChakraMove)
    //    Ensure_GlowObjects();

    Clear_DeadPoints(timeDelta);

    _uvFlowTime += timeDelta;

    const float ownerMoveSpeed = Compute_OwnerMoveSpeed(timeDelta);

    if (_isEmitting && ownerMoveSpeed >= _minOwnerMoveSpeed)
    {
        Update_LineOffsets(timeDelta);

        for (auto& channel : _channels)
        {
            Matrix boneWorld = Matrix::Identity;
            if (!Try_GetBoneWorldMatrix(channel.boneName, boneWorld))
                continue;

            for (auto& line : channel.lines)
            {
                const Vec3 worldOffset = Vec3::TransformNormal(line.localOffset, boneWorld);
                const Vec3 centerPos = boneWorld.Translation() + worldOffset;

                const Vec3 prevCenterPos = line.points.empty()
                    ? centerPos - Get_Owner()->Get_Transform()->Get_WorldForward() * 0.1f
                    : (line.points.front().point.topPos + line.points.front().point.bottomPos) * 0.5f;

                FTrailData data{};
                data.point = Make_TrailPoint(centerPos, prevCenterPos, channel.width);
                data.life = _lifespan;

                line.points.push_front(data);
            }
        }
    }

    Upload_LineBuffers();
}

HRESULT CharkraMove_Component::Render()
{
    if (!_shader || !_texture)
        return S_OK;

    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    Matrix identity = Matrix::Identity;
    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &identity), E_FAIL);
    CHECK_FAILED(_texture->Bind_SRV(_shader, "g_Texture", 0), E_FAIL);

    CHECK_FAILED(_shader->Bind_RawValue("g_Time", &_uvFlowTime, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_TintColor", &_trailTintColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_EmissiveStrength", &_trailEmissiveStrength, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_UVScrollX", &_uvScrollX, sizeof(float)), E_FAIL);

    for (auto& channel : _channels)
    {
        for (auto& line : channel.lines)
        {
            if (line.points.size() < 2 || !line.viBuffer)
                continue;

            CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
            CHECK_FAILED(line.viBuffer->Bind_Resources(), E_FAIL);
            CHECK_FAILED(line.viBuffer->Render(), E_FAIL);
        }
    }

    return S_OK;
}


bool CharkraMove_Component::Try_GetBoneWorldMatrix(const string& boneName, Matrix& outBoneWorld)
{
    auto owner = Get_Owner();
    if (!owner)
        return false;

    auto model = owner->Get_Component<Model>();
    if (!model)
        return false;

    const Matrix* socketMatrix = model->Get_SocketBoneMatrixPtr(boneName);
    if (!socketMatrix)
        return false;

    outBoneWorld = (*socketMatrix) * owner->Get_Transform()->Get_WorldMatrix();
    return true;
}

FTrailPoint CharkraMove_Component::Make_TrailPoint(const Vec3& centerPos, const Vec3& prevCenterPos, float width)
{
    const Matrix viewInv = GAME->Get_Transform(ETransformState::View)->Invert();
    const Vec3 camPos = viewInv.Translation();

    Vec3 moveDir = centerPos - prevCenterPos;
    if (moveDir.LengthSquared() < 0.0001f)
        moveDir = Get_Owner()->Get_Transform()->Get_WorldForward();
    moveDir.Normalize();

    Vec3 toCam = camPos - centerPos;
    if (toCam.LengthSquared() < 0.0001f)
        toCam = Vec3::Up;
    toCam.Normalize();

    Vec3 side = moveDir.Cross(toCam);
    if (side.LengthSquared() < 0.0001f)
        side = Vec3::Right;
    side.Normalize();

    FTrailPoint point{};
    point.topPos = centerPos + side * (width * 0.5f);
    point.bottomPos = centerPos - side * (width * 0.5f);

    return point;
}

Vec3 CharkraMove_Component::Pick_RandomLocalOffset(float jitterRadius) const
{
    return Vec3(
        Utils::RandomRange(-jitterRadius, jitterRadius),
        Utils::RandomRange(-jitterRadius, jitterRadius),
        Utils::RandomRange(-jitterRadius, jitterRadius));
}

void CharkraMove_Component::Update_LineOffsets(float timeDelta)
{
    for (auto& channel : _channels)
    {
        for (auto& line : channel.lines)
        {
            line.retargetTimer -= timeDelta;

            if (line.retargetTimer <= 0.f)
            {
                line.targetLocalOffset = channel.baseLocalOffset + Pick_RandomLocalOffset(channel.jitterRadius);
                line.retargetTimer = _retargetInterval;
            }

            line.localOffset = Vec3::Lerp(line.localOffset, line.targetLocalOffset, min(1.f, timeDelta * 18.f));
        }
    }
}

void CharkraMove_Component::Clear_DeadPoints(float timeDelta)
{
    for (auto& channel : _channels)
    {
        for (auto& line : channel.lines)
        {
            for (auto iter = line.points.begin(); iter != line.points.end();)
            {
                iter->life -= timeDelta;
                iter = (iter->life <= 0.f) ? line.points.erase(iter) : ++iter;
            }
        }
    }
}

void CharkraMove_Component::Upload_LineBuffers()
{
    for (auto& channel : _channels)
    {
        for (auto& line : channel.lines)
        {
            if (!line.viBuffer)
                continue;

            deque<FTrailPoint> drawPoints;
            for (const auto& data : line.points)
                drawPoints.push_back(data.point);

            line.viBuffer->Update_Trail(drawPoints);
        }
    }
}

float CharkraMove_Component::Compute_OwnerMoveSpeed(float timeDelta)
{
    auto owner = Get_Owner();
    if (!owner || timeDelta <= 0.f)
        return 0.f;

    const Vec3 currentPos = owner->Get_Transform()->Get_WorldPosition();

    if (!_hasPrevOwnerWorldPos)
    {
        _prevOwnerWorldPos = currentPos;
        _hasPrevOwnerWorldPos = true;
        return 0.f;
    }

    const float distance = Vec3::Distance(currentPos, _prevOwnerWorldPos);
    _prevOwnerWorldPos = currentPos;

    return distance / timeDelta;
}

void CharkraMove_Component::Start_DefaultChakraMove()
{
    vector<FTrailChannelDesc> channelDescs;

    // 왼발
    FTrailChannelDesc leftChannel{};
    leftChannel.boneName = _settings.leftBoneName;
    leftChannel.width = _settings.trailWidth;
    leftChannel.lineCount = max(1, _settings.trailLineCountPerFoot);
    leftChannel.baseLocalOffset = Vec3::Zero;
    leftChannel.jitterRadius = _settings.trailJitterRadius;
    channelDescs.push_back(leftChannel);

    // 오른발
    FTrailChannelDesc rightChannel{};
    rightChannel.boneName = _settings.rightBoneName;
    rightChannel.width = _settings.trailWidth;
    rightChannel.lineCount = max(1, _settings.trailLineCountPerFoot);
    rightChannel.baseLocalOffset = Vec3::Zero;
    rightChannel.jitterRadius = _settings.trailJitterRadius;
    channelDescs.push_back(rightChannel);

    Start_ChakraMove(channelDescs, _settings.trailLifespan, _settings.minOwnerMoveSpeed);

    //Ensure_GlowObjects();

    _hasStartedDefaultChakraMove = true;
}

void CharkraMove_Component::Ensure_GlowObjects()
{
    if (_settings.glowEffectName.empty())
        return;

    if (_leftGlowObject.expired())
        _leftGlowObject = Spawn_GlowObject(_settings.leftBoneName, _settings.leftGlowOffset);

    if (_rightGlowObject.expired())
        _rightGlowObject = Spawn_GlowObject(_settings.rightBoneName, _settings.rightGlowOffset);
}

void CharkraMove_Component::Cleanup_GlowObjects()
{
    auto leftGlow = _leftGlowObject.lock();
    if (!leftGlow || leftGlow->Is_Destroy())
        _leftGlowObject.reset();

    auto rightGlow = _rightGlowObject.lock();
    if (!rightGlow || rightGlow->Is_Destroy())
        _rightGlowObject.reset();
}

Shared<AttachedEffectObject> CharkraMove_Component::Spawn_GlowObject(const string& boneName, const Vec3& localOffset)
{
    auto owner = Get_Owner();
    if (!owner)
        return nullptr;

    auto model = owner->Get_Component<Model>();
    if (!model)
        return nullptr;

    const Matrix* socketMatrix = model->Get_SocketBoneMatrixPtr(boneName);
    if (!socketMatrix)
        return nullptr;

    AttachedEffectObject::FAttachedEffectObjectDesc effectDesc{};
    effectDesc.effectAssetName = _settings.glowEffectName;
    effectDesc.loopOverride = true;

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_ATTACHED_EFFECT,
        GAME->Current_Level(),
        TEXT("Layer_Effect"),
        &effectDesc);

    auto attachedEffect = dynamic_pointer_cast<AttachedEffectObject>(spawned);
    if (!attachedEffect)
        return nullptr;

    attachedEffect->Set_Owner(owner);

    attachedEffect->Attach_To_Bone(
        model.get(),
        owner->Get_Transform(),
        boneName,
        localOffset,
        _settings.glowLocalRotation,
        _settings.glowLocalScale);

    return attachedEffect;
}

Shared<CharkraMove_Component> CharkraMove_Component::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<CharkraMove_Component>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CharkraMove_Component");
        return nullptr;
    }

    return instance;
}

Shared<Component> CharkraMove_Component::Clone(void* arg)
{
    auto clone = make_shared<CharkraMove_Component>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : CharkraMove_Component");
        return nullptr;
    }

    return clone;
}

void CharkraMove_Component::Free()
{
    Stop_ChakraMove();

    Component::Free();
}
