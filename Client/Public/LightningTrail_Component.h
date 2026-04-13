#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class VIBuffer_Trail;
class Shader;
class Texture;
NS_END

NS_BEGIN(Client)

class LightningTrail_Component : public Component
{
    GENERATED_COMPONENT(LightningTrail_Component, Protocol::COMPONENT_TYPE_LIGHTNING_TRAIL)

public:
    struct FTrailData
    {
        FTrailPoint point;
        float life = 0.f; 
    };

    struct FLineData
    {
        deque<FTrailData> points;             
        Shared<VIBuffer_Trail> viBuffer;      
        Vec3 localOffset = Vec3::Zero;        
        Vec3 targetLocalOffset = Vec3::Zero;  
        float retargetTimer = 0.f;            
    };

public:
    explicit LightningTrail_Component(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit LightningTrail_Component(const LightningTrail_Component& rhs);
    virtual ~LightningTrail_Component() = default;

    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void BeginPlay() override;

    void Update_LightningTrail(float timeDelta);

    HRESULT Render();

    void Start_LightningTrail(const string& boneName, float lifespan = 0.18f, float width = 0.16f, uint32 lineCount = 5);
    void Stop_LightningTrail();
    bool Has_ActiveLightningTrail() const;

private:
    bool Try_GetBoneWorldMatrix(Matrix& outBoneWorld);
    FTrailPoint Make_TrailPoint(const Vec3& centerPos, const Vec3& prevCenterPos);
    Vec3 Pick_RandomLocalOffset() const;

    void Update_LineOffsets(float timeDelta);
    void Prune_DeadPoints(float timeDelta);
    void Upload_LineBuffers();

private:
    vector<FLineData> _lines;          
    Shared<Shader> _shader;            
    Shared<Texture> _texture;          

    bool _isEmitting = false;          
    string _boneName = "";             
    float _lifespan = 0.18f;           
    float _width = 0.16f;              
    float _jitterRadius = 0.35f;       // 왼손 주변 랜덤 흔들림 반경
    float _retargetInterval = 0.035f;  // 번개 목표 오프셋을 다시 뽑는 간격

public:
    static Shared<LightningTrail_Component> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
