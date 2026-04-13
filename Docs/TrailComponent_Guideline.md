# TrailComponent 완벽 구현 가이드 (Engine & Client 통합본)

이 가이드는 치도리 궤적, 검기 등을 구현하기 위해 엔진 단의 **동적 버텍스 버퍼(Dynamic Vertex Buffer)**부터 클라이언트 단의 **트레일 렌더러 컴포넌트**까지 모든 구현을 담고 있습니다.

> [!TIP]
> 뼈 이름(Bone Name)을 2개 입력하면 일반적인 검기(칼날의 위쳐/아래)로 동작하며, 1개만 입력하면 고정 폭(Width)을 가진 치도리/파티클 궤적으로 아름답게 동작합니다!

---

## 0. Enum.proto 컴포넌트 타입 추가

가장 먼저 `Enum.proto`에 트레일 렌더링용 컴포넌트 타입을 추가해야 합니다. 이 값들은 동적 버텍스 버퍼와 실제 사용될 컴포넌트를 만들 때 리플렉션 및 팩토리 매크로에 쓰입니다.

### [수정 대상] Server/Protobuf/Protocol/Enum.proto
```protobuf
enum ComponentType {
    // ... 기존 항목들 ...
    COMPONENT_TYPE_VIBUFFER_TRAIL = 1xx; // 적절한 빈 번호 사용
    COMPONENT_TYPE_TRAIL = 1xx;
}
```

> [!WARNING]
> 파일 수정 후 같은 폴더에 있는 `GenProto.bat`을 실행하여 C++ 프로토콜 파일(`Enum.pb.h`, `Enum.pb.cc` 등)을 새로고침해 주세요. 이를 생략하면 빌드 시 `Protocol::COMPONENT_TYPE_...` 식별자를 찾지 못해 컴파일 에러가 발생합니다.

---

## 1. Engine 영역 추가 파일

트레일은 길이가 실시간으로 변하므로 `D3D11_USAGE_DYNAMIC`을 사용하는 `VIBuffer_Trail`을 엔진에 추가해야 합니다.

### [추가] Engine/Public/VIBuffer_Trail.h
```cpp
#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

// 트레일을 구성할 정점 하나하나의 데이터 구조
struct FTrailPoint
{
    Vec3 topPos;    // 위쪽 가장자리 좌표
    Vec3 bottomPos; // 아래쪽 가장자리 좌표
};

class ENGINE_DLL VIBuffer_Trail final : public VIBuffer
{
    GENERATED_COMPONENT(VIBuffer_Trail, Protocol::COMPONENT_TYPE_VIBUFFER_TRAIL)

public:
    explicit VIBuffer_Trail(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Trail(const VIBuffer_Trail& rhs);
    virtual ~VIBuffer_Trail() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    // 매 프레임 살아있는 트레일 포인트들을 받아 버텍스 버퍼를 동적 갱신합니다.
    HRESULT Update_Vertices(const std::deque<FTrailPoint>& points);

public:
    static Shared<VIBuffer_Trail> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* pArg) override;
    virtual void Free() override;

private:
    uint32 _maxPoints = 200; // 버퍼 최대 길이 방어용
};

NS_END
```

### [추가] Engine/Private/VIBuffer_Trail.cpp
```cpp
#include "pch.h"
#include "VIBuffer_Trail.h"
#include "Component_Factory.h"

NS_BEGIN(Engine)

// 엔진 프로토타입 등록 매크로 (프로젝트 규칙에 맞춰 수정)
// REGISTER_COMPONENT_FACTORY(VIBuffer_Trail, Protocol::COMPONENT_TYPE_VIBUFFER_TRAIL)

VIBuffer_Trail::VIBuffer_Trail(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer(device, context)
{
}

VIBuffer_Trail::VIBuffer_Trail(const VIBuffer_Trail& rhs)
    : VIBuffer(rhs)
{
}

HRESULT VIBuffer_Trail::Initialize_Prototype()
{
    _numVertexBuffers = 1;
    _maxPoints = 200; // 최대 200개의 점 기록 (400 버텍스)
    _numVertices = _maxPoints * 2; 
    _vertexStride = sizeof(FVertexTex);
    
    // TriangleList 방식으로 그리기 위해 6개의 인덱스(2개의 삼각형) 단위 구성
    _numIndices = (_maxPoints - 1) * 6;
    _indexStride = sizeof(uint16);
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // 1. 동적 버텍스 버퍼 생성 (DYNAMIC, WRITE)
    D3D11_BUFFER_DESC vertexDesc{};
    vertexDesc.ByteWidth = _vertexStride * _numVertices;
    vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    CHECK_FAILED(_device->CreateBuffer(&vertexDesc, nullptr, _vertexBuffer.GetAddressOf()), E_FAIL);

    // 2. 인덱스 버퍼 생성 (미리 최대 사이즈만큼 정적으로 세팅)
    vector<uint16> indices;
    indices.resize(_numIndices);

    for (uint32 i = 0; i < _maxPoints - 1; ++i)
    {
        indices[i * 6 + 0] = i * 2 + 0;
        indices[i * 6 + 1] = i * 2 + 1;
        indices[i * 6 + 2] = i * 2 + 2;

        indices[i * 6 + 3] = i * 2 + 1;
        indices[i * 6 + 4] = i * 2 + 3;
        indices[i * 6 + 5] = i * 2 + 2;
    }

    D3D11_BUFFER_DESC indexDesc{};
    indexDesc.ByteWidth = _indexStride * _numIndices;
    indexDesc.Usage = D3D11_USAGE_DEFAULT;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA indexData{};
    indexData.pSysMem = indices.data();

    CHECK_FAILED(_device->CreateBuffer(&indexDesc, &indexData, _indexBuffer.GetAddressOf()), E_FAIL);

    // 최초에는 그릴 게 없으니 0으로 초기화
    _numIndices = 0; 
    return S_OK;
}

HRESULT VIBuffer_Trail::Initialize(void* arg)
{
    return S_OK;
}

HRESULT VIBuffer_Trail::Update_Vertices(const std::deque<FTrailPoint>& points)
{
    if (points.size() < 2)
    {
        _numIndices = 0; // 점이 2개가 안되면 그리지 않음
        return S_OK;
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(_context->Map(_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        VTXTEX* vertices = static_cast<VTXTEX*>(mapped.pData);
        uint32 limit = min(static_cast<uint32>(points.size()), _maxPoints);

        for (uint32 i = 0; i < limit; ++i)
        {
            // UV의 U값: 앞부분(0)부터 꼬리(1)까지 이어지게 맵핑
            float uRatio = static_cast<float>(i) / (limit - 1); 

            vertices[i * 2 + 0].position = points[i].topPos;
            vertices[i * 2 + 0].texCoord = Vec2(uRatio, 0.f);

            vertices[i * 2 + 1].position = points[i].bottomPos;
            vertices[i * 2 + 1].texCoord = Vec2(uRatio, 1.f);
        }

        _context->Unmap(_vertexBuffer.Get(), 0);

        // VIBuffer가 실제로 그릴_numIndices 갯수를 갱신해줌
        _numIndices = (limit - 1) * 6;
    }
    return S_OK;
}

Shared<VIBuffer_Trail> VIBuffer_Trail::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<VIBuffer_Trail>(device, context);
    if (FAILED(instance->Initialize_Prototype())) return nullptr;
    return instance;
}

Shared<Component> VIBuffer_Trail::Clone(void* pArg)
{
    auto instance = make_shared<VIBuffer_Trail>(*this);
    if (FAILED(instance->Initialize(pArg))) return nullptr;
    return instance;
}

void VIBuffer_Trail::Free()
{
    VIBuffer::Free();
}

NS_END
```

---

## 2. Client 영역 추가 파일

플레이어(또는 무기)에 달아줄 트레일 컨트롤용 컴포넌트입니다.

### [추가] Client/Public/Trail_Component.h
```cpp
#pragma once

#include "Component.h"
#include "VIBuffer_Trail.h"

NS_BEGIN(Client)

class Trail_Component : public Component
{
    GENERATED_COMPONENT(Trail_Component, Protocol::COMPONENT_TYPE_TRAIL)

    struct FTrailData
    {
        Engine::FTrailPoint point;
        float life;
    };

public:
    explicit Trail_Component(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Trail_Component(const Trail_Component& rhs);
    virtual ~Trail_Component() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void BeginPlay() override;

    void Update(float timeDelta);
    HRESULT Render();

    // 트레일 시작 (뼈를 하나만 넣으면 defaultWidth로 임의의 폭을 형성)
    void Start_Trail(const string& topBone, const string& bottomBone, float lifespan, float defaultWidth = 3.f);
    void Stop_Trail();

private:
    std::deque<FTrailData>          _points;
    Shared<Engine::VIBuffer_Trail>  _viBuffer;
    Shared<class Shader>            _shader;
    Shared<class Texture>           _texture; // 트레일 이미지 (치도리, 검기 등)

    bool   _isEmitting = false;
    string _topBoneName = "";
    string _bottomBoneName = "";
    float  _lifespan = 0.5f;
    float  _defaultWidth = 3.f;

public:
    static Shared<Trail_Component> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
```

### [추가] Client/Private/Trail_Component.cpp
```cpp
#include "pch.h"
#include "Trail_Component.h"

#include "GameObject.h"
#include "Model.h"
#include "Transform.h"
#include "Shader.h"
#include "Texture.h"

NS_BEGIN(Client)

// 컴포넌트 팩토리 등록
// REGISTER_COMPONENT_FACTORY(Trail_Component, Protocol::COMPONENT_TYPE_TRAIL)

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
    // 쓸 셰이더와 텍스쳐, 버퍼를 준비합니다.
    _viBuffer = static_pointer_cast<Engine::VIBuffer_Trail>(
        GAME->Clone_Component(0/*Static*/, Protocol::COMPONENT_TYPE_VIBUFFER_TRAIL)
    );
    
    // 일반 텍스쳐 셰이더 혹은 이펙트 셰이더 활용 (구조에 맞게 셰이더 타입 변경 가능)
    _shader = static_pointer_cast<Shader>(
        GAME->Clone_Component(0/*Static*/, Protocol::COMPONENT_TYPE_SHADER_VTXTEX)
    );

    // 테스트용 트레일 이미지
    _texture = static_pointer_cast<Texture>(
        GAME->Clone_Component(0/*Static*/, Protocol::COMPONENT_TYPE_TEXTURE_TRAIL)
    );

    return S_OK;
}

void Trail_Component::BeginPlay()
{
    Component::BeginPlay();
}

void Trail_Component::Update(float timeDelta)
{
    // 1. 수명 차감 및 꼬리 자르기
    for (auto iter = _points.begin(); iter != _points.end();)
    {
        iter->life -= timeDelta;
        if (iter->life <= 0.f)
            iter = _points.erase(iter); // 수명 만료된 점 제거, erase()는 다음 iter 반환
        else
            ++iter;
    }

    // 2. 새로운 점 방출
    if (_isEmitting)
    {
        if (auto owner = Get_Owner())
        {
            if (auto model = owner->Get_Component<Model>())
            {
                Vec3 topPos(0,0,0), bottomPos(0,0,0);
                
                // Top Bone 가져오기
                if (const Matrix* topMat = model->Get_SocketBoneMatrixPtr(_topBoneName))
                {
                    Matrix worldMat = (*topMat) * owner->Get_Transform()->Get_WorldMatrix();
                    topPos = worldMat.Translation();
                    
                    // Bottom Bone이 없다면 (뼈 1개 모드)
                    if (_bottomBoneName.empty())
                    {
                        // 임의로 로컬 Y축(Up) 방향으로 폭을 줍니다. (치도리에 적합)
                        Vec3 boneUp = worldMat.Up();
                        boneUp.Normalize();
                        bottomPos = topPos - (boneUp * _defaultWidth);
                    }
                    // Bottom Bone이 있다면 (뼈 2개 모드 - 검기)
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
    }

    // 3. 버퍼 동적 업데이트 (화면에 렌더링될 실제 모양 갱신)
    if (_viBuffer)
    {
        std::deque<Engine::FTrailPoint> pointsToDraw;
        for (const auto& p : _points)
            pointsToDraw.push_back(p.point);
            
        _viBuffer->Update_Vertices(pointsToDraw);
    }
}

HRESULT Trail_Component::Render()
{
    if (_points.size() < 2 || !_viBuffer || !_shader)
        return S_OK;

    // View, Proj 세팅 및 트레일 텍스처 삽입 후 렌더링
    CHECK_FAILED(_shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);
    
    // 트레일은 이미 월드 좌표로 계산되어 있으므로 WorldMatrix는 Identity
    Matrix identity = Matrix::Identity;
    CHECK_FAILED(_shader->Bind_Matrix("g_WorldMatrix", &identity), E_FAIL);

    if (_texture)
        CHECK_FAILED(_texture->Bind_ShaderResource(_shader, "g_DiffuseTexture", 0), E_FAIL);

    CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL); // 알파 블렌딩 패스
    CHECK_FAILED(_viBuffer->Render(), E_FAIL);

    return S_OK;
}

void Trail_Component::Start_Trail(const string& topBone, const string& bottomBone, float lifespan, float defaultWidth)
{
    _topBoneName = topBone;
    _bottomBoneName = bottomBone;
    _lifespan = lifespan;
    _defaultWidth = defaultWidth;
    _isEmitting = true;
    _points.clear(); // 새로 켤 때 궤적 초기화 여부 (필요시 제거 가능)
}

void Trail_Component::Stop_Trail()
{
    _isEmitting = false;
    // 포인트는 수명(life)에 의해 서서히 지워짐
}

Shared<Trail_Component> Trail_Component::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Trail_Component>(device, context);
    if (FAILED(instance->Initialize_Prototype())) return nullptr;
    return instance;
}
Shared<Component> Trail_Component::Clone(void* arg)
{
    auto clone = make_shared<Trail_Component>(*this);
    if (FAILED(clone->Initialize(arg))) return nullptr;
    return clone;
}
void Trail_Component::Free()
{
    Component::Free();
}

NS_END
```

---

## 3. Notify용 애니메이션 상태 (`ANS_Trail.h/cpp`)

이제 더 이상 Start와 End를 나눌 필요가 없습니다! 모션이 끝나는 시간대에 맞게 상태(State) 노티파이를 달아놓기만 하면, 중간에 캔슬되거나 모션이 바뀌더라도 `Execute_End()`가 꼬리를 자르고 아름답게 마무리해 줍니다.

### [추가] Client/Public/ANS_Trail.h
```cpp
#pragma once
#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_Trail : public AnimNotifyState
{
    GENERATED_BODY(ANS_Trail)

public:
    string Get_TypeName() const override { return "ANS_Trail"; }

protected:
    virtual bool Register_Properties() override;

public:
    virtual void Execute_Begin(const FAnimNotifyContext& context) override;
    virtual void Execute_Tick(const FAnimNotifyContext& context) override {}
    virtual void Execute_End(const FAnimNotifyContext& context) override;

private:
    string _topBoneName = "";
    string _bottomBoneName = ""; // 옵션
    float  _lifespan = 0.5f;
    float  _width = 3.f; // 뼈가 하나일때 굵기
};

NS_END
```

### [추가] Client/Private/ANS_Trail.cpp
```cpp
#include "pch.h"
#include "ANS_Trail.h"
#include "AnimNotify_Factory.h"
#include "Trail_Component.h"
#include "GameObject.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(ANS_Trail)
IMPLEMENT_REFLECTION(ANS_Trail)

bool ANS_Trail::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_Trail";

    PROPERTY_STRING_JSON("위쪽 뼈 이름", "top_bone", _topBoneName);
    PROPERTY_STRING_JSON("아랫쪽 뼈 이름(옵션)", "bot_bone", _bottomBoneName);
    PROPERTY_FLOAT_JSON("궤적 잔류 시간", "lifespan", _lifespan, 0.1f, 5.0f);
    PROPERTY_FLOAT_JSON("굵기 (뼈 1개일때)", "width", _width, 0.1f, 10.0f);

    return true;
}

void ANS_Trail::Execute_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    if (auto trailCom = context.owner->Get_Component<Trail_Component>())
    {
        trailCom->Start_Trail(_topBoneName, _bottomBoneName, _lifespan, _width);
    }
}

void ANS_Trail::Execute_End(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    if (auto trailCom = context.owner->Get_Component<Trail_Component>())
    {
        trailCom->Stop_Trail();
    }
}

NS_END
```

---

## 4. Point Notify용 `AN_Trail_Start` / `AN_Trail_Stop`

치도리처럼 **충전 → 돌진 → 히트** 등 애니메이션이 여러 개로 쪼개진 경우,  
`ANS` 하나로 구간을 잡을 수 없으므로 **시작/종료를 별도 Point Notify**로 분리합니다.

```
[충전 애니] AN_Trail_Start  →  Trail_Component::Start_Trail()
[히트 애니] AN_Trail_Stop   →  Trail_Component::Stop_Trail()
```

> [!NOTE]
> `ANS_Trail`과 `AN_Trail_Start/Stop`은 **공존** 가능합니다.  
> 검 휘두르기처럼 1개 애니 안에서 끝나면 `ANS_Trail`, 치도리처럼 멀티 애니라면 `AN_Trail_Start/Stop`을 사용하세요.

### [추가] Client/Public/AN_Trail_Start.h
```cpp
#pragma once
#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_Trail_Start : public AnimNotify
{
    GENERATED_BODY(AN_Trail_Start)

public:
    string Get_TypeName() const override { return "AN_Trail_Start"; }

protected:
    virtual bool Register_Properties() override;

public:
    // 해당 애니메이션 프레임에 도달하면 즉시 트레일을 시작합니다.
    virtual void Execute(const FAnimNotifyContext& context) override;

private:
    string _topBoneName    = "";    // 위쪽 추적 뼈 이름
    string _bottomBoneName = "";    // 아래쪽 추적 뼈 이름 (없으면 1뼈 모드)
    float  _lifespan       = 0.5f;  // 각 점의 잔류 시간 (초)
    float  _width          = 3.f;   // 1뼈 모드일 때 트레일 굵기
};

NS_END
```

### [추가] Client/Private/AN_Trail_Start.cpp
```cpp
#include "pch.h"
#include "AN_Trail_Start.h"
#include "AnimNotify_Factory.h"
#include "Trail_Component.h"
#include "GameObject.h"

NS_BEGIN(Client)

// Point Notify는 REGISTER_ANIM_NOTIFY 사용 (State는 REGISTER_ANIM_NOTIFY_STATE)
REGISTER_ANIM_NOTIFY(AN_Trail_Start)
IMPLEMENT_REFLECTION(AN_Trail_Start)

bool AN_Trail_Start::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_Trail_Start";

    PROPERTY_STRING_JSON("위쪽 뼈 이름",          "top_bone",  _topBoneName);
    PROPERTY_STRING_JSON("아랫쪽 뼈 이름(옵션)",  "bot_bone",  _bottomBoneName);
    PROPERTY_FLOAT_JSON("궤적 잔류 시간",          "lifespan",  _lifespan, 0.1f, 5.0f);
    PROPERTY_FLOAT_JSON("굵기 (뼈 1개일때)",       "width",     _width,    0.1f, 10.0f);

    return true;
}

void AN_Trail_Start::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    if (auto trailCom = context.owner->Get_Component<Trail_Component>())
    {
        trailCom->Start_Trail(_topBoneName, _bottomBoneName, _lifespan, _width);
    }
}

NS_END
```

---

### [추가] Client/Public/AN_Trail_Stop.h
```cpp
#pragma once
#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_Trail_Stop : public AnimNotify
{
    GENERATED_BODY(AN_Trail_Stop)

public:
    string Get_TypeName() const override { return "AN_Trail_Stop"; }

    // 프로퍼티 없음 — 항상 트레일을 종료만 합니다.
    // Stop 후에도 기존 점들은 lifespan에 따라 서서히 사라집니다.
    virtual void Execute(const FAnimNotifyContext& context) override;
};

NS_END
```

### [추가] Client/Private/AN_Trail_Stop.cpp
```cpp
#include "pch.h"
#include "AN_Trail_Stop.h"
#include "AnimNotify_Factory.h"
#include "Trail_Component.h"
#include "GameObject.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(AN_Trail_Stop)
IMPLEMENT_REFLECTION(AN_Trail_Stop)

void AN_Trail_Stop::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    if (auto trailCom = context.owner->Get_Component<Trail_Component>())
    {
        // 방출만 멈춤 — 남은 점들은 lifespan대로 자연 소멸
        trailCom->Stop_Trail();
    }
}

NS_END
```

---

## 5. 사용 시나리오 요약

| 상황 | 사용할 Notify |
|---|---|
| 검 휘두르기, 일반 공격 (1개 애니 내 구간) | `ANS_Trail` |
| 치도리 충전~히트 (여러 애니로 분리) | `AN_Trail_Start` + `AN_Trail_Stop` |
| 모션 캔슬 시 자동 종료 필요 | `ANS_Trail` (Execute_End 자동 호출됨) |
| 특정 프레임 정확히 켜고 끄고 싶을 때 | `AN_Trail_Start` + `AN_Trail_Stop` |
