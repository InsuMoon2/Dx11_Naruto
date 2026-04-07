#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class Model;
class MovementComponent;
NS_END

NS_BEGIN(Client)


class SmearEffect_Component : public Component
{
    GENERATED_COMPONENT(SmearEffect_Component, Protocol::COMPONENT_TYPE_SMEAR_EFFECT)

public:
    // [추가] 스미어 전체 기본 동작값을 담는 설정 구조체
    struct FSmearSettings
    {
        bool active = false;                         
        float captureTimer = 0.f;                    
        float captureInterval = 0.02f;               
        float lifespan = 0.14f;                      
        float smearLength = 1.4f;                    
        float stretchScale = 1.15f;                  
        Vec4 baseColor = Vec4(0.05f, 0.08f, 0.25f, 1.f);   
        Vec4 edgeColor = Vec4(0.45f, 0.65f, 1.0f, 1.f);    
    };

    struct FRenderableSnapshot
    {
        Shared<Model> model;                                // 실제 렌더할 Skeletal 모델 컴포넌트
        Matrix worldMatrix = Matrix::Identity;              // 스냅샷 시점의 기준 월드행렬
        EMeshVertexType modelType = EMeshVertexType::END;   // 현재는 SkeletalMesh만 사용
        vector<Matrix> boneMatrices;                        // Skeletal 본 행렬 스냅샷
    };

    struct FSmearSnapshot
    {
        float life = 0.f;                           
        float maxLife = 0.f;                        
        Vec3 smearDir = Vec3::Forward;              
        vector<FRenderableSnapshot> renderables;    
    };

public:
    explicit SmearEffect_Component(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SmearEffect_Component(const SmearEffect_Component& rhs);
    virtual ~SmearEffect_Component() = default;

public:
    HRESULT Initialize(void* arg) override;

    void BeginPlay() override;

    void Update_Smear(float timeDelta);

    HRESULT Render();

public:
    void Start_Smear(float captureInterval, float lifespan, float smearLength, Vec4 baseColor, Vec4 edgeColor);
    void Stop_Smear();

    bool Has_ActiveSmear() const { return !_snapshots.empty(); }

private:
    void Capture_CurrentSnapshot();

    Vec3 Resolve_SmearDirection() const;
    HRESULT Render_Snapshot(const FSmearSnapshot& snapshot);

private:
    FSmearSettings _settings;                    
    vector<FSmearSnapshot> _snapshots;           

    Shared<Shader> _smearSkelShader;               

    Weak<Transform> _ownerTransform;                
    Weak<GameObject> _ownerObject;                  
    Weak<MovementComponent> _ownerMovement;

public:
    static Shared<SmearEffect_Component> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};
    
NS_END

