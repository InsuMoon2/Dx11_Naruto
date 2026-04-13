#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class VIBuffer_Trail;
class Shader;
class Texture;
NS_END

NS_BEGIN(Client)

class Trail_Component : public Component
{
    GENERATED_COMPONENT(Trail_Component, Protocol::COMPONENT_TYPE_TRAIL)

public:
    struct FTrailData
    {
        FTrailPoint point;
        float life;
    };

public:
    explicit Trail_Component(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Trail_Component(const Trail_Component& rhs);
    virtual ~Trail_Component() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    BeginPlay() override;

    void    Update_Trail(float timeDelta);
    HRESULT Render();

public:
    void Start_Trail(const string& topBone, const string& bottomBone, float lifespan, float defulatWidth = 3.f);
    void Stop_Trail();

	bool Has_ActiveTrail() const { return _isEmitting || !_points.empty();}

private:
    deque<FTrailData>       _points;
    Shared<VIBuffer_Trail>  _viBuffer;
    Shared<Shader>          _shader;
    Shared<Texture>         _texture;

private:
    bool _isEmitting = false;
    string _topBoneName = "";
    string _bottomBoneName = "";

    float _lifespan = 0.5f;
    float _defaultWidth = 3.f;

public:
    static Shared<Trail_Component> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
