#pragma once

#include "VIBuffer_Instance.h"
#include "EffectAsset_Types.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer_Particle_Point : public VIBuffer_Instance
{
    GENERATED_COMPONENT(VIBuffer_Particle_Point, Protocol::COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT)

public:
    enum class EMoveMode : uint8
    {
        Drop = 0,   
        Spread = 1, 
        Static = 2, 
    };

    struct FParticlePointDesc : public VIBuffer_Instance::FInstanceDesc
    {
        Vec3    pivot = Vec3::Zero;
        Vec2    speed = Vec2(1.f, 1.f);
        Vec2    lifeTime = Vec2(1.f, 1.f);
        bool    isLoop = false;
        EEffectPointSpawnShape spawnShape = EEffectPointSpawnShape::Box; 
        float   spawnRadius = 1.f;              
        float   spawnInnerRadius = 0.f;         
        float   spawnHeight = 1.f;              

        EMoveMode moveMode = EMoveMode::Static; 
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
    void    Update_Static(float timeDelta);

private:
    Vec3    Build_SpawnOffset(const FParticlePointDesc& desc) const;
    Vec3    Resolve_SpreadDirection(const Vec3& spawnOffset) const;
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
    EMoveMode                       _moveMode = EMoveMode::Static;      
    EEffectPointSpawnShape          _spawnShape = EEffectPointSpawnShape::Box;
    float                           _spawnRadius = 1.f;                 
    float                           _spawnInnerRadius = 0.f;            
    float                           _spawnHeight = 1.f;                 

public:
    static Shared<VIBuffer_Particle_Point> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};

NS_END
