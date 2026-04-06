# 범용 3D 잔상 이펙트 가이드 (Component 기반 아키텍처)

## 📌 Summary
- **검토 결과**: 원본 나루토 게임은 잔상을 단순히 2D 사각형으로 띄우지 않고, **실제 3D 캐릭터 모델의 스냅샷을 찍어 "고스트(Ghost)" 형태로 투명하게 렌더링**하는 방식을 사용 중입니다.
- **아키텍처**: 설계 확장을 고려하여 플레이어 본체에 하드코딩하지 않고, **`GhostEffectComponent`**라는 범용 부품(Component)으로 분리합니다. 
- **장점**: 이 컴포넌트만 부착하면 **플레이어뿐만 아니라 보스 몬스터, 분신술 NPC 등** 어떤 객체라도 `Start_GhostEffect()` 한 줄로 이펙트를 쓸 수 있습니다.
- **범용화**: 별도의 `ANS_AfterImage` 범용 노티파이를 만들어, Payload 값 편집만으로 모든 애니메이션에서 자유롭게 색상, 짧기, 굵기를 변경합니다.

---

## 🚀 Step 1. 잔상 전용 특수 셰이더 추가

가장 먼저 캐릭터의 본체는 지정한 단색으로 투명하게 채우고 외곽선(림라이트)만 빛나게 하는 **고스트 셰이더**를 만듭니다.

`Client/Bin/Shaders/Shader_GhostAfterImage.hlsl` 파일을 새로 생성하고 아래 내용을 넣습니다.

```hlsl
// ==========================================
// Shader_GhostAfterImage.hlsl
// ==========================================
#include "Engine_Shader_Defines.hlsli"

// 전역 셰이더(Engine_Shader_Defines.hlsli)에 아래 항목이 없다면 주석을 풀고 사용하세요.
// matrix g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;
// float4 g_vCamPosition;

matrix g_BoneMatrices[256];

float4 g_GhostColor = float4(0.02f, 0.02f, 0.02f, 1.0f); // 몸체 내부 색상
float4 g_RimColor   = float4(0.1f, 0.4f, 1.0f, 1.0f);    // 테두리 빛 색상
float  g_GhostAlpha = 1.0f;                              // 전체 투명도 (페이드 아웃용)
float  g_RimPower   = 2.0f;                              // 림라이트 두께 및 강도

struct VS_IN
{
    float3 vPosition    : POSITION;
    float3 vNormal      : NORMAL;
    float2 vTexcoord    : TEXCOORD;
    uint4  vBlendIndex  : BLENDINDEX;
    float4 vBlendWeight : BLENDWEIGHT;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float3 vWorldPos : TEXCOORD0;
    float3 vNormal   : NORMAL;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    // 뼈대 애니메이션 가중치 유지 (과거의 포즈 Freeze 용도)
    matrix boneMatrix = 
        g_BoneMatrices[In.vBlendIndex.x] * In.vBlendWeight.x +
        g_BoneMatrices[In.vBlendIndex.y] * In.vBlendWeight.y +
        g_BoneMatrices[In.vBlendIndex.z] * In.vBlendWeight.z +
        g_BoneMatrices[In.vBlendIndex.w] * In.vBlendWeight.w;

    matrix worldMatrix = mul(boneMatrix, g_WorldMatrix);

    Out.vPosition = mul(float4(In.vPosition, 1.f), worldMatrix);
    Out.vPosition = mul(Out.vPosition, g_ViewMatrix);
    Out.vPosition = mul(Out.vPosition, g_ProjMatrix);

    Out.vWorldPos = Out.vPosition.xyz;
    Out.vNormal   = normalize(mul(float4(In.vNormal, 0.f), worldMatrix).xyz);

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vWorldPos : TEXCOORD0;
    float3 vNormal   : NORMAL;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    float3 viewDir = normalize(g_vCamPosition.xyz - In.vWorldPos);
    
    // 림라이트 공식 (가장자리에만 강하게)
    float rimFactor = 1.0f - max(dot(viewDir, normalize(In.vNormal)), 0.0f);
    rimFactor = pow(smoothstep(0.0f, 1.0f, rimFactor), g_RimPower);

    // 파라미터 색상 섞기
    float4 finalColor = lerp(g_GhostColor, g_RimColor, rimFactor);
    finalColor.a *= g_GhostAlpha; // 수명이 닳으면서 투명해집니다.

    Out.vColor = finalColor;
    return Out;
}

technique11 DefaultTechnique
{
    pass GhostPass 
    {
        SetRasterizerState(RS_Default);
        // SrcAlpha + InvSrcAlpha 등 엔진의 범용 투명 블렌드 스테이트를 적용
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        SetDepthStencilState(DS_ZTest_NoWrite, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}
```

---

## 🛠️ Step 2. Model.h 에 관절(Bone) 추출 Getter 추가

잔상은 "과거의 포즈"를 그대로 얼려두어야(Freeze) 합니다. 이를 위해 월드 행렬뿐만 아니라 `Model` 내부의 애니메이션 결과물(`_boneMatrices`)도 복사본을 가져올 수 있어야 합니다.

`Engine/Public/Model.h` 파일의 `public:` 영역에 함수를 한 줄 추가합니다.

```cpp
public:
    // [추가] 고스트 잔상(스냅샷) 생성 등을 위해 현재 프레임의 뼈대 최종 SRT 행렬 번들을 반환합니다.
    const vector<Matrix>& Get_BoneMatrices() const { return _boneMatrices; }
```


---

## ⚙️ Step 3. 핵심! GhostEffect_Component 추가

본체 클래스를 깔끔하게 유지하기 위해 컴포넌트로 모든 기능을 빼냅니다. (쥬신 엔진 컴포넌트 룰 준수!)

### `GhostEffect_Component.h`
```cpp
#pragma once
#include "Component.h"
#include "Shader.h"
#include "Model.h"

NS_BEGIN(Client)

// 잔상 효과를 관리하는 단일 컴포넌트입니다.
class GhostEffect_Component : public Component
{
    // ★ 쥬신 엔진 컴포넌트 필수 매크로 및 ID
    GENERATED_COMPONENT(GhostEffect_Component, Protocol::COMPONENT_TYPE_GHOST_EFFECT)

public:
    explicit GhostEffect_Component(
        ComPtr<Device> device, 
        ComPtr<DeviceContext> context);

    // ★ Prototype 복제 패턴을 위한 필수 복사 생성자
    explicit GhostEffect_Component(const GhostEffect_Component& rhs);
    virtual ~GhostEffect_Component() = default;

public:
    virtual HRESULT Initialize(void* arg) override;
    virtual void BeginPlay() override;
    
    // 컴포넌트 클래스엔 부모에 Update 가상함수가 없으므로 override를 붙이지 않습니다! (본체가 수동 호출)
    void Update(float timeDelta);
    HRESULT Render(); 

public:
    // 외부 애니메이션 노티파이나 로직에서 켜는 스위치
    void Start_GhostEffect(float interval, float lifespan, Vec4 color, Vec4 rimColor);
    void Stop_GhostEffect();

    // 현재 화면에 남아있는 잔상이 존재하는지 여부 (투명 렌더큐 등록용)
    bool Has_ActiveGhosts() const { return !_ghostSnapshots.empty(); }

private:
```
    struct FGhostSnapshot
    {
        float lifespan = 0.f;        
        float maxLifespan = 0.f;     
        Matrix worldMatrix;          
        vector<Matrix> boneMatrices; 
        Vec4 color;
        Vec4 rimColor;
    };

    struct FGhostSettings
    {
        bool active = false;
        float captureTimer = 0.f;      
        float captureInterval = 0.05f; 
        float defaultLifespan = 0.3f;  
        Vec4 color = Vec4(0.02f, 0.02f, 0.02f, 1.f);
        Vec4 rimColor = Vec4(0.1f, 0.4f, 1.0f, 1.f); 
    };

    FGhostSettings _settings;
    vector<FGhostSnapshot> _ghostSnapshots;
    Shared<Shader> _ghostShader;

    // 본체의 Transform과 Model에 접근하기 위한 약한 포인터
    weak_ptr<Transform> _ownerTransform;
    weak_ptr<Model> _ownerModel;

public:
    static Shared<GhostEffect_Component> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* arg) override;
};

NS_END
```

### `GhostEffect_Component.cpp`
```cpp
#include "pch.h"
#include "GhostEffect_Component.h"
#include "GameObject.h" // Get_Owner() 용도

GhostEffect_Component::GhostEffect_Component(
    ComPtr<Device> device, 
    ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

// ★ 복사 생성자 구현
GhostEffect_Component::GhostEffect_Component(
    const GhostEffect_Component& rhs)
    : Component(rhs)
{
}

HRESULT GhostEffect_Component::Initialize(void* arg)
{
    // ★ 쥬신 엔진 컨벤션: 셰이더는 무조건 Prototype을 Clone해서 쓴다!
    // 엑셀에 등록하신 ID인 Protocol::COMPONENT_TYPE_SHADER_GHOST_EFFECT 를 통계로 가져옵니다.
    _ghostShader = static_pointer_cast<Shader>(GAME->Clone_Component(
        LEVEL_STATIC, 
        Protocol::COMPONENT_TYPE_SHADER_GHOST_EFFECT));
        
    CHECK_NULL(_ghostShader, E_FAIL);

    return S_OK;
}

void GhostEffect_Component::BeginPlay()
{
    Component::BeginPlay();

    auto owner = Get_Owner();
    if (owner)
    {
        _ownerTransform = owner->Get_Transform();
        // Model 키값이 다를 수 있으니 각자 엔진 설정에 맞게 가져옵니다.
        _ownerModel = owner->Get_Component<Model>(); 
    }
}

void GhostEffect_Component::Start_GhostEffect(
    float interval, 
    float lifespan, 
    Vec4 color, 
    Vec4 rimColor)
{
    _settings.active = true;
    _settings.captureTimer = 0.f; 
    _settings.captureInterval = interval;
    _settings.defaultLifespan = lifespan;
    _settings.color = color;
    _settings.rimColor = rimColor;
}

void GhostEffect_Component::Stop_GhostEffect()
{
    _settings.active = false;
}

void GhostEffect_Component::Update(float timeDelta)
{
    // 1. 기존 잔상들 수명 줄이고 만료 시 삭제
    for (auto it = _ghostSnapshots.begin(); it != _ghostSnapshots.end();)
    {
        it->lifespan -= timeDelta;
        if (it->lifespan <= 0.f)
            it = _ghostSnapshots.erase(it);
        else
            ++it;
    }

    // 2. 이펙트 생성 중이라면 일정 주기마다 스냅샷 찍기
    if (_settings.active && !_ownerTransform.expired() && !_ownerModel.expired())
    {
        _settings.captureTimer += timeDelta;
        if (_settings.captureTimer >= _settings.captureInterval)
        {
            _settings.captureTimer = 0.f;

            auto transform = _ownerTransform.lock();
            auto model = _ownerModel.lock();

            FGhostSnapshot snapshot;
            snapshot.lifespan = _settings.defaultLifespan;
            snapshot.maxLifespan = _settings.defaultLifespan;
            snapshot.worldMatrix = transform->Get_WorldMatrix();
            // ★ 순간의 뼈대 관절 각도들을 복사하여 보관! 
            snapshot.boneMatrices = model->Get_BoneMatrices(); 
            snapshot.color = _settings.color;
            snapshot.rimColor = _settings.rimColor;

            _ghostSnapshots.push_back(snapshot);
        }
    }
}

HRESULT GhostEffect_Component::Render()
{
    if (_ghostSnapshots.empty() || !_ghostShader || _ownerModel.expired())
        return S_OK;

    auto model = _ownerModel.lock();

    // 공통 매트릭스 설정
    _ghostShader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _ghostShader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));
    GAME->Bind_CamPosition(_ghostShader, "g_vCamPosition");

    for (const auto& ghost : _ghostSnapshots)
    {
        _ghostShader->Bind_Matrix("g_WorldMatrix", &ghost.worldMatrix);
        _ghostShader->Bind_RawValue("g_GhostColor", &ghost.color, sizeof(Vec4));
        _ghostShader->Bind_RawValue("g_RimColor", &ghost.rimColor, sizeof(Vec4));
        
        // 알파 페이드 (시간 경과에 따라 1 -> 0)
        float alpha = ghost.lifespan / ghost.maxLifespan;
        _ghostShader->Bind_RawValue("g_GhostAlpha", &alpha, sizeof(float));

        // 해당 스냅샷을 찍을 때의 본(뼈대) 행렬들 바인딩!
        _ghostShader->Bind_RawValue(
            "g_BoneMatrices", 
            ghost.boneMatrices.data(), 
            sizeof(Matrix) * ghost.boneMatrices.size());

        // 셰이더 첫번째 Pass 적용 후 곧바로 렌더
        _ghostShader->Begin(0); 

        // 텍스쳐는 따로 바인드하지 않고 오직 Vertex 모양만 활용
        for (auto& mesh : model->Get_Meshes())
        {
            mesh->Render();
        }
    }
    return S_OK;
}

Shared<GhostEffect_Component> GhostEffect_Component::Create(
    ComPtr<Device> device, 
    ComPtr<DeviceContext> context)
{
    auto instance = make_shared<GhostEffect_Component>(device, context);
    if (FAILED(instance->Initialize(nullptr))) return nullptr;
    return instance;
}

Shared<Component> GhostEffect_Component::Clone(void* arg)
{
    auto instance = make_shared<GhostEffect_Component>(*this);
    if (FAILED(instance->Initialize(arg))) return nullptr;
    return instance;
}
```

---

## 🏃 Step 4. Player에서 Component 갖다 붙이기

컴포넌트를 만들었으니 플러그 꽂듯 부착만 하면 끝납니다.

**1. 조립 및 Update 호출 (`Player`)**
```cpp
// 1. Ready_Components 쯤에 부착
CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_GHOST_EFFECT, _ghostEffectCom), E_FAIL); 

// 2. Update() 에서 오버라이드가 아닌 수동으로 Update 함수 호출을 넘겨줌
if (_ghostEffectCom) _ghostEffectCom->Update(timeDelta);
```

**2. 렌더 큐 등록 (`Player::Late_Update(...)`)**
잔상이 하나라도 생겨있다면 투명으로 렌더되게끔 `Blend` 목록에 넣어줍니다.
```cpp
// 본체 뒤쪽에 잔상이 투명하게 보이려면 ERenderGroup::Blend 로 추가되어야 합니다.
if (_ghostEffectCom && _ghostEffectCom->Has_ActiveGhosts())
{
    GAME->Add_RenderGroup(ERenderGroup::Blend, this->GetSharedPtr());
}
```

**3. 그려주기 (`Player::Render()`)**
```cpp
// (필요 시 엔진 구조에 따라) Blend 패스를 그리고 있을 경우에만 호출하도록 분기
// if (현재 Render 패스가 Blend라면)
if (_ghostEffectCom)
{
    _ghostEffectCom->Render();
}
```

---

## 🎯 Step 5. 범용 노티파이 스테이트 (ANS_GhostEffect) 연결

가장 핵심이 되는 `AnimNotifyState` 입니다. 앞으로 대쉬, 점프, 필살기 등 이 툴만 연동해 payload만 바꾸면 모두 쓸 수 있습니다.

### `ANS_GhostEffect.h`
```cpp
#pragma once
#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_GhostEffect : public AnimNotifyState
{
    GENERATED_BODY(ANS_GhostEffect)

public:
    string Get_TypeName() const override { return "ANS_GhostEffect"; }

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context)  override {}
    void On_End(const FAnimNotifyContext& context)   override;

public:
    json Serialize_Payload() const override;
    void Deserialize_Payload(const json& payload) override;

private:
    float _captureInterval = 0.05f; 
    float _lifespan = 0.3f;         
    Vec4  _ghostColor = Vec4(0.02f, 0.02f, 0.02f, 1.f);
    Vec4  _rimColor = Vec4(0.1f, 0.4f, 1.0f, 1.f);
};

NS_END
```

### `ANS_GhostEffect.cpp`
```cpp
#include "pch.h"
#include "ANS_GhostEffect.h"
#include "GameObject.h"
#include "GhostEffect_Component.h" // 바로 직접 가져오기!
#include "AnimNotify_Factory.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_GhostEffect);
IMPLEMENT_REFLECTION(ANS_GhostEffect);

bool ANS_GhostEffect::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_GhostEffect";
    info.properties.clear();

    // 에디터에서 손쉽게 고칠 수 있도록 속성 등록
    PROPERTY_FLOAT("Capture Interval", _captureInterval);
    PROPERTY_FLOAT("Ghost Lifespan", _lifespan);

    return true;
}

void ANS_GhostEffect::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    // 누가 불렀건 관계없이 컴포넌트만 꺼내서 쓴다! (보스든 플레이어든 상관 없음)
    auto ghostCom = context.owner->Get_Component<GhostEffect_Component>();
    if (ghostCom)
    {
        ghostCom->Start_GhostEffect(_captureInterval, _lifespan, _ghostColor, _rimColor);
    }
}

void ANS_GhostEffect::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    auto ghostCom = context.owner->Get_Component<GhostEffect_Component>();
    if (ghostCom)
    {
        ghostCom->Stop_GhostEffect();
    }
}

json ANS_GhostEffect::Serialize_Payload() const
{
    json j;
    j["interval"] = _captureInterval;
    j["lifespan"] = _lifespan;
    j["ghost_color"] = { _ghostColor.x, _ghostColor.y, _ghostColor.z, _ghostColor.w };
    j["rim_color"] = { _rimColor.x, _rimColor.y, _rimColor.z, _rimColor.w };
    return j;
}

void ANS_GhostEffect::Deserialize_Payload(const json& payload)
{
    if (payload.contains("interval")) _captureInterval = payload["interval"];
    if (payload.contains("lifespan")) _lifespan = payload["lifespan"];
    
    if (payload.contains("ghost_color")) {
        _ghostColor.x = payload["ghost_color"][0];
        _ghostColor.y = payload["ghost_color"][1];
        _ghostColor.z = payload["ghost_color"][2];
        _ghostColor.w = payload["ghost_color"][3];
    }
    if (payload.contains("rim_color")) {
        _rimColor.x = payload["rim_color"][0];
        _rimColor.y = payload["rim_color"][1];
        _rimColor.z = payload["rim_color"][2];
        _rimColor.w = payload["rim_color"][3];
    }

    AnimNotifyState::Deserialize_Payload(payload);
}
```

---

## 🎨 최종 연동 (JSON)

가까운 `TesetNormal.animnotify.json` 쪽 `Dash_Start` 클립에 이 노티파이를 부여합니다.

```json
{
  "clip_name": "SK_CHR_NormalModel|CustomMan_Aerial_Dash_Start",
  "notify_states": [
    {
      "start_sec": 0.0,
      "duration_sec": 0.3,
      "type": "ANS_GhostEffect",
      "payload": {
          "interval": 0.03,            
          "lifespan": 0.25,
          "ghost_color": [0.02, 0.02, 0.02, 1.0],
          "rim_color": [0.1, 0.4, 1.0, 1.0]     
      }
    }
  ]
}
```

이 방식은 컴포넌트로 완전히 분리되었기 때문에, 향후 **어떤 NPC**에게도 `Add_Component()`만 해주면 똑같은 혜택을 누릴 수 있습니다! 코딩 설계가 매우 깔끔하고 안정적입니다.
