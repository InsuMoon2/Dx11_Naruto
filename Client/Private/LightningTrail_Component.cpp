#include "pch.h"
#include "LightningTrail_Component.h"
#include "GameObject.h"
#include "Model.h"
#include "Transform.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Trail.h"

LightningTrail_Component::LightningTrail_Component(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

LightningTrail_Component::LightningTrail_Component(const LightningTrail_Component& rhs)
    : Component(rhs)
{
}

HRESULT LightningTrail_Component::Initialize_Prototype()
{
    return S_OK;
}

HRESULT LightningTrail_Component::Initialize(void* arg)
{
    _shader = static_pointer_cast<Shader>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_SHADER_VTXTEX));
    CHECK_NULL(_shader, E_FAIL);

    _texture = static_pointer_cast<Texture>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_TEXTURE_TRAIL));
    CHECK_NULL(_texture, E_FAIL);

    return S_OK;
}

void LightningTrail_Component::BeginPlay()
{
    Component::BeginPlay();
}

void LightningTrail_Component::Start_LightningTrail(const string& boneName, float lifespan, float width, uint32 lineCount)
{
    _boneName = boneName;
    _lifespan = lifespan;
    _width = width;
    _isEmitting = true;

    _lines.clear();
    _lines.resize(max<uint32>(1, lineCount));

    for (uint32 i = 0; i < _lines.size(); ++i)
    {
        auto& line = _lines[i];

        line.viBuffer = static_pointer_cast<VIBuffer_Trail>(
            GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_VIBUFFER_TRAIL));

        line.localOffset = (i == 0) ? Vec3::Zero : Pick_RandomLocalOffset();
        line.targetLocalOffset = line.localOffset;
        line.retargetTimer = 0.f;
        line.points.clear();
    }
}

void LightningTrail_Component::Stop_LightningTrail()
{
    _isEmitting = false;
}

bool LightningTrail_Component::Has_ActiveLightningTrail() const
{
    if (_isEmitting)
        return true;

    for (const auto& line : _lines)
    {
        if (!line.points.empty())
            return true;
    }

    return false;
}

void LightningTrail_Component::Update_LightningTrail(float timeDelta)
{
    Prune_DeadPoints(timeDelta);

    if (_isEmitting)
    {
        Matrix boneWorld = Matrix::Identity;
        if (Try_GetBoneWorldMatrix(boneWorld))
        {
            Update_LineOffsets(timeDelta);

            for (auto& line : _lines)
            {
                const Vec3 worldOffset = Vec3::TransformNormal(line.localOffset, boneWorld);
                const Vec3 centerPos = boneWorld.Translation() + worldOffset;
                const Vec3 prevCenterPos = line.points.empty()
                    ? centerPos - Get_Owner()->Get_Transform()->Get_WorldForward() * 0.1f
                    : (line.points.front().point.topPos + line.points.front().point.bottomPos) * 0.5f;

                FTrailData data{};
                data.point = Make_TrailPoint(centerPos, prevCenterPos);
                data.life = _lifespan;

                line.points.push_front(data);
            }
        }
    }

    Upload_LineBuffers();
}

HRESULT LightningTrail_Component::Render()
{
    if (!_shader || !_texture)
        return S_OK;

    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    Matrix identity = Matrix::Identity;
    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &identity), E_FAIL);
    CHECK_FAILED(_texture->Bind_SRV(_shader, "g_Texture", 0), E_FAIL);

    for (auto& line : _lines)
    {
        if (line.points.size() < 2 || !line.viBuffer)
            continue;

        CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(line.viBuffer->Bind_Resources(), E_FAIL);
        CHECK_FAILED(line.viBuffer->Render(), E_FAIL);
    }

    return S_OK;
}

bool LightningTrail_Component::Try_GetBoneWorldMatrix(Matrix& outBoneWorld)
{
    auto owner = Get_Owner();
    if (!owner)
        return false;

    auto model = owner->Get_Component<Model>();
    if (!model)
        return false;

    const Matrix* socketMatrix = model->Get_SocketBoneMatrixPtr(_boneName);
    if (!socketMatrix)
        return false;

    outBoneWorld = (*socketMatrix) * owner->Get_Transform()->Get_WorldMatrix();
    return true;
}

FTrailPoint LightningTrail_Component::Make_TrailPoint(const Vec3& centerPos, const Vec3& prevCenterPos)
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
    point.topPos = centerPos + side * (_width * 0.5f);
    point.bottomPos = centerPos - side * (_width * 0.5f);

    return point;
}

Vec3 LightningTrail_Component::Pick_RandomLocalOffset() const
{
    return Vec3(
        Utils::RandomRange(-_jitterRadius, _jitterRadius),
        Utils::RandomRange(-_jitterRadius, _jitterRadius),
        Utils::RandomRange(-_jitterRadius, _jitterRadius));
}

void LightningTrail_Component::Update_LineOffsets(float timeDelta)
{
    for (auto& line : _lines)
    {
        line.retargetTimer -= timeDelta;

        if (line.retargetTimer <= 0.f)
        {
            line.targetLocalOffset = Pick_RandomLocalOffset();
            line.retargetTimer = _retargetInterval;
        }

        line.localOffset = Vec3::Lerp(line.localOffset, line.targetLocalOffset, min(1.f, timeDelta * 18.f));
    }
}

void LightningTrail_Component::Prune_DeadPoints(float timeDelta)
{
    for (auto& line : _lines)
    {
        for (auto iter = line.points.begin(); iter != line.points.end();)
        {
            iter->life -= timeDelta;
            iter = (iter->life <= 0.f) ? line.points.erase(iter) : ++iter;
        }
    }
}

void LightningTrail_Component::Upload_LineBuffers()
{
    for (auto& line : _lines)
    {
        if (!line.viBuffer)
            continue;

        deque<FTrailPoint> drawPoints;
        for (const auto& data : line.points)
            drawPoints.push_back(data.point);

        line.viBuffer->Update_Trail(drawPoints);
    }
}

Shared<LightningTrail_Component> LightningTrail_Component::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<LightningTrail_Component>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : LightningTrail_Component");
        return nullptr;
    }

    return instance;
}

Shared<Component> LightningTrail_Component::Clone(void* arg)
{
    auto clone = make_shared<LightningTrail_Component>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : LightningTrail_Component");
        return nullptr;
    }

    return clone;
}

void LightningTrail_Component::Free()
{
    Component::Free();
}
