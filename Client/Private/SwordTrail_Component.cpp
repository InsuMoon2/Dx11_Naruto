#include "pch.h"
#include "SwordTrail_Component.h"
#include "GameObject.h"
#include "Model.h"
#include "Transform.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Trail.h"
#include "ContainerObject.h"
#include "Weapon.h"

SwordTrail_Component::SwordTrail_Component(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

SwordTrail_Component::SwordTrail_Component(const SwordTrail_Component& rhs)
    : Component(rhs)
{
}

HRESULT SwordTrail_Component::Initialize_Prototype()
{
    return S_OK;
}

HRESULT SwordTrail_Component::Initialize(void* arg)
{
    _viBuffer = static_pointer_cast<VIBuffer_Trail>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_VIBUFFER_TRAIL));

    _shader = static_pointer_cast<Shader>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_SHADER_SWORD_TRAIL));

    _texture = static_pointer_cast<Texture>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_TEXTURE_SWORD_TRAIL));

    return S_OK;
}

void SwordTrail_Component::BeginPlay()
{
    Component::BeginPlay();
}

void SwordTrail_Component::Update_SwordTrail(float timeDelta)
{
    _uvFlowTime += timeDelta;

    for (auto iter = _points.begin(); iter != _points.end();)
    {
        iter->life -= timeDelta;

        if (iter->life <= 0.f)
            iter = _points.erase(iter);
        else
            ++iter;
    }

    if (_isEmitting)
    {
        Vec3 topPos = Vec3::Zero;
        Vec3 bottomPos = Vec3::Zero;

        if (!Try_GetWeaponTrailWorldPoints(topPos, bottomPos))
        {
            if (auto owner = Get_Owner())
            {
                auto model = owner->Get_Component<Model>();
                CHECK_NULL(model);

                const Matrix* topMat = model->Get_SocketBoneMatrixPtr(_topBoneName);
                if (topMat)
                {
                    Matrix worldMat = (*topMat) * owner->Get_Transform()->Get_WorldMatrix();
                    topPos = worldMat.Translation();

                    if (_bottomBoneName.empty())
                    {
                        Vec3 boneUp = worldMat.Up();
                        boneUp.Normalize();
                        bottomPos = topPos - (boneUp * _defaultWidth);
                    }
                    else if (const Matrix* botMat = model->Get_SocketBoneMatrixPtr(_bottomBoneName))
                    {
                        Matrix botWorld = (*botMat) * owner->Get_Transform()->Get_WorldMatrix();
                        bottomPos = botWorld.Translation();
                    }
                }
            }
        }

        if (topPos != Vec3::Zero || bottomPos != Vec3::Zero)
        {
            FTrailData newData{};
            newData.point.topPos = topPos;
            newData.point.bottomPos = bottomPos;
            newData.life = _lifespan;

            _points.push_back(newData);
        }
    }

    if (_viBuffer)
    {
        deque<FTrailPoint> pointsToDraw;

        for (const auto& trail : _points)
            pointsToDraw.push_back(trail.point);

        _viBuffer->Update_Trail(pointsToDraw);
    }
}


HRESULT SwordTrail_Component::Render()
{
    if (_points.size() < 2 || !_viBuffer || !_shader)
        return S_OK;

    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    Matrix identity = Matrix::Identity;
    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &identity), E_FAIL);

    if (_texture)
        CHECK_FAILED(_texture->Bind_SRV(_shader, "g_Texture", _textureIndex), E_FAIL);

    CHECK_FAILED(_shader->Bind_RawValue("g_Time", &_uvFlowTime, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_TintColor", &_trailTintColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_EmissiveStrength", &_trailEmissiveStrength, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_UVScrollX", &_uvScrollX, sizeof(float)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_MaskCut", &_maskCut, sizeof(float)), E_FAIL);

    CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
    CHECK_FAILED(_viBuffer->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_viBuffer->Render(), E_FAIL);

    return S_OK;
}

void SwordTrail_Component::Start_SwordTrail(const string& topBone, const string& bottomBone, float lifespan, float defaultWidth, int32 textureIndex)
{
    _topBoneName    = topBone;
    _bottomBoneName = bottomBone;
    _lifespan       = lifespan;
    _defaultWidth   = defaultWidth;
    _isEmitting     = true;
    _uvFlowTime     = 0.f;

    _textureIndex = textureIndex;

    _points.clear();
}

void SwordTrail_Component::Stop_SwordTrail()
{
    _isEmitting = false;
}

void SwordTrail_Component::Clear_SwordTrail()
{
    _isEmitting = false;
    _points.clear();

    if (_viBuffer)
    {
        deque<FTrailPoint> emptyPoints;
        _viBuffer->Update_Trail(emptyPoints);
    }
}

bool SwordTrail_Component::Try_GetWeaponTrailWorldPoints(Vec3& outTopWorld, Vec3& outBottomWorld)
{
    auto owner = Get_Owner();
    if (!owner)
        return false;

    auto container = dynamic_pointer_cast<ContainerObject>(owner);
    if (!container)
        return false;

    auto weaponBase = container->Get_PartObject(ContainerObject::EPartSlot::Weapon);
    if (!weaponBase)
        return false;

    auto weapon = dynamic_pointer_cast<Weapon>(weaponBase);
    if (!weapon)
        return false;

    Vec3 rootWorld = Vec3::Zero;
    Vec3 tipWorld = Vec3::Zero;

    if (!weapon->Get_SwordTrailWorldPoints(rootWorld, tipWorld))
        return false;

    outTopWorld = tipWorld;
    outBottomWorld = rootWorld;

    return true;
}

Shared<SwordTrail_Component> SwordTrail_Component::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SwordTrail_Component>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : SwordTrail_Component");
        return nullptr;
    }

    return instance;
}

Shared<Component> SwordTrail_Component::Clone(void* arg)
{
    auto clone = make_shared<SwordTrail_Component>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : SwordTrail_Component");
        return nullptr;
    }

    return clone;
}

void SwordTrail_Component::Free()
{
    Component::Free();
}
