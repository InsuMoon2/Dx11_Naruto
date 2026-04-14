#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class VIBuffer_Trail;
class Shader;
class Texture;

NS_END

NS_BEGIN(Client)

class AttachedEffectObject;

class CharkraMove_Component : public Component
{
    GENERATED_COMPONENT(CharkraMove_Component, Protocol::COMPONENT_TYPE_CHAKRA_MOVE)

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

    struct FTrailChannelDesc
    {
        string  boneName = "";
        float   width = 0.46f;
        uint32  lineCount = 2;
        Vec3    baseLocalOffset = Vec3::Zero;
        float   jitterRadius = 0.05f; // 퍼질 반경
    };

    struct FTrailChannelData
    {
        string  boneName = "";
        vector<FLineData> lines;
        Vec3    baseLocalOffset = Vec3::Zero;
        float   width = 0.46f;
        float   jitterRadius = 0.05f;
    };

    struct FChakraMoveSettings
    {
        string  leftBoneName      = "LeftFoot";
        string  rightBoneName     = "RightFoot";
        string  glowEffectName    = "Chakra_Move";

        Vec3    leftGlowOffset = Vec3(0.f, 0.02f, 0.f); 
        Vec3    rightGlowOffset = Vec3(0.f, 0.02f, 0.f);
        Vec3    glowLocalRotation = Vec3::Zero;         
        Vec3    glowLocalScale = Vec3(0.3f, 0.3f, 0.3f);

        float   trailLifespan = 0.4f;                  
        float   trailWidth = 0.26f;                     
        int32   trailLineCountPerFoot = 2;              
        float   trailJitterRadius = 0.05f;              
        float   minOwnerMoveSpeed = 5.f;                
    };

public:
    explicit CharkraMove_Component(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit CharkraMove_Component(const CharkraMove_Component& rhs);
    virtual ~CharkraMove_Component() = default;

    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

    void    Update_ChakraMove(float timeDelta);
    HRESULT Render();

public:
    void Start_ChakraMove(const vector<FTrailChannelDesc>& channelDescs, float lifespan = 0.16f, float minOwnerMoveSpeed = 5.f);
    void Stop_ChakraMove();                                                                                                       
    bool Has_ActiveChakraMove() const;           

private:
    bool Try_GetBoneWorldMatrix(const string& boneName, Matrix& outBoneWorld);
    FTrailPoint Make_TrailPoint(const Vec3& centerPos, const Vec3& prevCenterPos, float width);
    Vec3 Pick_RandomLocalOffset(float jitterRadius) const;

    void Update_LineOffsets(float timeDelta);
    void Clear_DeadPoints(float timeDelta);
    void Upload_LineBuffers();

    float Compute_OwnerMoveSpeed(float timeDelta);

    // 데이터 세팅
    void Start_DefaultChakraMove();
    void Ensure_GlowObjects();
    void Cleanup_GlowObjects();
    Shared<AttachedEffectObject> Spawn_GlowObject(const string& boneName, const Vec3& localOffset); 

private:
    vector<FTrailChannelData> _channels;       
    Shared<Shader> _shader;                    
    Shared<Texture> _texture;                  

    bool    _isEmitting = false;                  
    float   _lifespan = 0.36f;                   
    float   _retargetInterval = 0.035f;          
    float   _minOwnerMoveSpeed = 5.f;          

    Vec3    _prevOwnerWorldPos = Vec3::Zero;      
    bool    _hasPrevOwnerWorldPos = false;

private:
    FChakraMoveSettings _settings;
    bool _hasStartedDefaultChakraMove = false;
    Weak<AttachedEffectObject> _leftGlowObject;
    Weak<AttachedEffectObject> _rightGlowObject;

    float   _uvFlowTime = 0.f;   
    Vec4    _trailTintColor = Vec4(0.18f, 0.42f, 1.00f, 1.f);
    float   _trailEmissiveStrength = 1.6f;

    float   _uvScrollX = -1.2f;

public:
    static Shared<CharkraMove_Component> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
