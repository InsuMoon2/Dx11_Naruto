#pragma once

#include "VIBuffer_Instance.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer_Particle_Point : public VIBuffer_Instance
{
    GENERATED_COMPONENT(VIBuffer_Particle_Point, Protocol::COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT)

public:
    enum class EMoveMode : uint8
    {
        Drop = 0,
        Spread = 1,
    };

    struct FParticlePointDesc : public VIBuffer_Instance::FInstanceDesc
    {
        Vec3    pivot = Vec3::Zero;
        Vec2    speed = Vec2(1.f, 1.f);
        Vec2    lifeTime = Vec2(1.f, 1.f);
        bool    isLoop = false;

        EMoveMode moveMode = EMoveMode::Drop;
    };

public:
    explicit VIBuffer_Particle_Point(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Particle_Point(const VIBuffer_Particle_Point& rhs);
    virtual ~VIBuffer_Particle_Point() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    HRESULT Bind_Resources() override;
    HRESULT Render() override;

    void    Update_Particles(float timeDelta);
    void    Update_Drop(float timeDelta);
    void    Update_Spread(float timeDelta);

private:
    HRESULT Build_Instances(const FParticlePointDesc& desc);
    HRESULT Upload_InstanceBuffer();

    void    Reset_Instance(uint32 index);

private:
    vector<VTXPARTICLE_INSTANCE>    _initialInstances;  // 초기 복구용
    vector<VTXPARTICLE_INSTANCE>    _instances;         // 현재 갱신 상태
    vector<float>                   _speeds;
    vector<Vec3>                    _directions;

    Vec3                            _pivot = Vec3::Zero;                
    bool                            _isLoop = false;                    
    EMoveMode                       _moveMode = EMoveMode::Drop;        

public:
    static Shared<VIBuffer_Particle_Point> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};

NS_END
