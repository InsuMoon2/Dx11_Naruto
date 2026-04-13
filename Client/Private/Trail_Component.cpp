#include "pch.h"
#include "Trail_Component.h"

#include "GameObject.h"
#include "Model.h"
#include "Transform.h"
#include "Shader.h"
#include "Texture.h"
#include "VIBuffer_Trail.h"

Trail_Component::Trail_Component(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Trail_Component::Trail_Component(const Trail_Component& rhs)
    : Component(rhs)
{
}

HRESULT Trail_Component::Initialize_Prototype()
{
    return S_OK;
}

HRESULT Trail_Component::Initialize(void* arg)
{
    _viBuffer = static_pointer_cast<VIBuffer_Trail>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_VIBUFFER_TRAIL));

     _shader = static_pointer_cast<Shader>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_SHADER_VTXTEX)
    );

    // 테스트용 트레일 이미지
    _texture = static_pointer_cast<Texture>(
        GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_TEXTURE_TRAIL)
    );

    return S_OK;
}

void Trail_Component::BeginPlay()
{
    Component::BeginPlay();
}

void Trail_Component::Update_Trail(float timeDelta)
{
    // 수명 차감
    for (auto iter = _points.begin(); iter != _points.end();)
    {
        iter->life -= timeDelta;

        if (iter->life <= 0.f)
            iter = _points.erase(iter);
        else
            ++iter;
    }

    // 새로운 점 방출
    if (_isEmitting)
    {
        if (auto owner = Get_Owner())
        {
            auto model = owner->Get_Component<Model>();
            CHECK_NULL(model);

            Vec3 topPos = Vec3::Zero;;
            Vec3 bottomPos = Vec3::Zero;

            // Top Bone 세팅
            const Matrix* topMat = model->Get_SocketBoneMatrixPtr(_topBoneName);
            if (topMat)
            {
                Matrix worldMat = (*topMat) * owner->Get_Transform()->Get_WorldMatrix();
                topPos = worldMat.Translation();

                // Bottom Bone이 없다면 (뼈 1개세팅 -> 치도리는 근데 1개아닌가)
                if (_bottomBoneName.empty())
                {
                    Vec3 boneUp = worldMat.Up();
                    boneUp.Normalize();
                    bottomPos = topPos - (boneUp * _defaultWidth);
                } // Bottom 뼈가 있다면
                else if (const Matrix* botMat = model->Get_SocketBoneMatrixPtr(_bottomBoneName))
                {
                    Matrix botWorld = (*botMat) * owner->Get_Transform()->Get_WorldMatrix();
                    bottomPos = botWorld.Translation();
                }

                FTrailData newData;
                newData.point.topPos = topPos;
                newData.point.bottomPos = bottomPos;
                newData.life = _lifespan;

                _points.push_back(newData);
            }
        }
    }

    if (_viBuffer)
    {
        deque<FTrailPoint> pointsToDraw;

        for (const auto& trail : _points)
        {
            pointsToDraw.push_back(trail.point);
        }

        _viBuffer->Update_Trail(pointsToDraw);
    }

}

HRESULT Trail_Component::Render()
{
    if (_points.size() < 2 || !_viBuffer || !_shader)
        return S_OK;

    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);
    
    Matrix identity = Matrix::Identity;
    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &identity), E_FAIL);

    if (_texture)
        CHECK_FAILED(_texture->Bind_SRV(_shader, "g_Texture", 0), E_FAIL);

    CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
	CHECK_FAILED(_viBuffer->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_viBuffer->Render(), E_FAIL);

    return S_OK;
}

void Trail_Component::Start_Trail(const string& topBone, const string& bottomBone, float lifespan, float defulatWidth)
{
    _topBoneName = topBone;
    _bottomBoneName = bottomBone;
    _lifespan = lifespan;
    _defaultWidth = defulatWidth;
    _isEmitting = true;

    _points.clear(); // 새로 켤 때 궤적 초기화 -> 이건 일단 테스트해보고
}

void Trail_Component::Stop_Trail()
{
    _isEmitting = false;
}

Shared<Trail_Component> Trail_Component::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Trail_Component>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Trail_Component");
        return nullptr;
    }

    return instance;
}

Shared<Component> Trail_Component::Clone(void* arg)
{
    auto clone = make_shared<Trail_Component>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : Trail_Component");
        return nullptr;
    }

    return clone;
}

void Trail_Component::Free()
{
    Component::Free();
}
