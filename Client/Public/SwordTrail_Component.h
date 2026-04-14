#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class VIBuffer_Trail;
class Shader;
class Texture;
NS_END

NS_BEGIN(Client)

class SwordTrail_Component : public Component
{
    GENERATED_COMPONENT(SwordTrail_Component, Protocol::COMPONENT_TYPE_SWORD_TRAIL)

public:
    struct FTrailData
    {
        FTrailPoint point;
        float life = 0.f;
    };

public:
    explicit SwordTrail_Component(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SwordTrail_Component(const SwordTrail_Component& rhs);
    virtual ~SwordTrail_Component() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

    void    Update_SwordTrail(float timeDelta);
    HRESULT Render();

public:
    void Start_SwordTrail(const string& topBone, const string& bottomBone,
        float lifespan, float defaultWidth = 3.f, int32 textureIndex = 0);

    void Stop_SwordTrail();
    bool Has_ActiveSwordTrail() const { return _isEmitting || !_points.empty(); }

    void Clear_SwordTrail();

private:
    bool Try_GetWeaponTrailWorldPoints(Vec3& outTopWorld, Vec3& outBottomWorld);

private:
    deque<FTrailData>       _points;
    Shared<VIBuffer_Trail>  _viBuffer;
    Shared<Shader>          _shader;
    Shared<Texture>         _texture;

    int32                  _textureIndex = 0;

private:
    bool    _isEmitting = false;                  
    string  _topBoneName = "";                    
    string  _bottomBoneName = "";                 
    float   _lifespan = 0.18f;                    
    float   _defaultWidth = 2.5f;                 

    float   _uvFlowTime = 0.f;
    float   _uvScrollX = -1.8f;                   
    Vec4    _trailTintColor = Vec4(0.78f, 0.88f, 1.0f, 1.f);
    float   _trailEmissiveStrength = 1.6f;       
    float   _maskCut = 0.65f;                    

public:
    static Shared<SwordTrail_Component> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
