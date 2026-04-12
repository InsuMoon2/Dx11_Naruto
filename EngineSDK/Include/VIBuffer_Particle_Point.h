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
        Drop = 0,   // 중력 방향으로 떨어지는 기본 파티클 모드
        Spread = 1, // 중심에서 바깥쪽으로 퍼지며 커지는 방사형 모드
        Static = 2, // 위치/크기 변화 없이 빌보드 이미지만 유지하는 정지 모드
    };

    struct FParticlePointDesc : public VIBuffer_Instance::FInstanceDesc
    {
        Vec3    pivot = Vec3::Zero;
        Vec2    speed = Vec2(1.f, 1.f);
        Vec2    lifeTime = Vec2(1.f, 1.f);
        bool    isLoop = false;
        EEffectPointSpawnShape spawnShape = EEffectPointSpawnShape::Box; // 인스턴스를 어떤 부피/형태에서 생성할지 정한다.
        float   spawnRadius = 1.f;              // Sphere/Cylinder/Ring 모드의 바깥 반경이다.
        float   spawnInnerRadius = 0.f;         // Sphere/Cylinder/Ring 모드의 안쪽 비움 반경이다.
        float   spawnHeight = 1.f;              // Cylinder 모드의 높이 범위다.

        EMoveMode moveMode = EMoveMode::Static; // 데칼/아이콘류 Point는 기본적으로 정지 빌보드로 시작한다.
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

    // 현재 설정된 move mode에 따라 각 Point 인스턴스의 수명/위치/크기를 갱신한다.
    void    Update_Particles(float timeDelta);
    // 눈/낙하 파티클처럼 아래로 내려가는 이동을 처리할 때 호출된다.
    void    Update_Drop(float timeDelta);
    // 폭발/분산처럼 중심에서 퍼져나가며 커지는 이동을 처리할 때 호출된다.
    void    Update_Spread(float timeDelta);
    // 라센간 데칼처럼 제자리에 유지되는 정지 빌보드를 갱신할 때 호출된다.
    void    Update_Static(float timeDelta);

private:
    // 스폰 형태 설정에 맞춰 인스턴스 시작 위치 오프셋을 생성할 때 호출한다.
    Vec3    Build_SpawnOffset(const FParticlePointDesc& desc) const;
    // Spread 모드에서 퍼져나갈 방향을 시작 위치 기준으로 계산할 때 호출한다.
    Vec3    Resolve_SpreadDirection(const Vec3& spawnOffset) const;
    HRESULT Build_Instances(const FParticlePointDesc& desc);
    HRESULT Upload_InstanceBuffer();

    // 수명 종료 후 루프 재생 시 개별 인스턴스를 초기 상태로 되돌릴 때 호출한다.
    void    Reset_Instance(uint32 index);

private:
    vector<VTXPARTICLE_INSTANCE>    _initialInstances;  // 초기 복구용
    vector<VTXPARTICLE_INSTANCE>    _instances;         // 현재 갱신 상태
    vector<float>                   _speeds;
    vector<Vec3>                    _directions;

    Vec3                            _pivot = Vec3::Zero;                // 모든 인스턴스가 생성될 기준 중심점이다.
    bool                            _isLoop = false;                    // 수명 종료 시 재스폰할지 여부다.
    EMoveMode                       _moveMode = EMoveMode::Static;      // 현재 Point 인스턴스들의 이동 방식이다.
    EEffectPointSpawnShape          _spawnShape = EEffectPointSpawnShape::Box; // 현재 Point 인스턴스들이 어떤 스폰 형태를 쓰는지 기억한다.
    float                           _spawnRadius = 1.f;                 // 현재 스폰 형태의 바깥 반경이다.
    float                           _spawnInnerRadius = 0.f;            // 현재 스폰 형태의 안쪽 비움 반경이다.
    float                           _spawnHeight = 1.f;                 // Cylinder 스폰에서 사용할 높이 범위다.

public:
    static Shared<VIBuffer_Particle_Point> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};

NS_END
