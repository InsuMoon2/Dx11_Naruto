# Dx11_Naruto 파티클 시스템 (Instancing) 통합 및 A to Z 가이드라인

본 문서는 하드웨어 인스턴싱 기반의 VtxParticleRect(눈/비)와 Geometry Shader를 결합한 VtxParticlePoint(폭발/스파크) 시스템을 현재 `Dx11_Naruto` 프로젝트 구조에 완벽하게 통합하기 위한 **자세한 완성형 코드 가이드라인**입니다. 요청하신 전역 `.hlsli` 통합 처리 및 모든 헤더/CPP 수정 사항을 생략 없이 포함합니다.

---

## 1. 셰이더 전역 관리 (`.hlsli`) 통합 (Engine 레벨)

모든 프로젝트(Client, Editor 등)에서 공통으로 쓰는 전역 행렬, 라이트, 카메라, 샘플러 변수들은 **Engine** 쪽에서 원본을 관리하고, `UpdateLib.bat`을 통해 클라이언트/에디터 쪽으로 복사하여 사용하는 것이 구조상 올바릅니다.

### 1-1. Engine에 `.hlsli` 추가 (변수명 통일)
기존 `Dx11_Naruto` 셰이더들을 확인해보면 애니메이션 메쉬(`Shader_VtxAnimMesh.hlsl`)에서는 `g_vLightDir`, `g_vCamPosition` 처럼 변수명 앞에 `v`가 붙어있고, 일반 메쉬(`Shader_VtxMesh.hlsl`)에서는 `g_LightDir`를 쓰며 심지어 `g_MtrlAmbiment`라는 오타가 존재합니다.

전역 파일로 통합할 때는 이 **이름표(변수명)들을 모두 하나로 통일**해야 정상 작동합니다. 여기서는 보편적으로 쓰고 있는 접두어 없는 이름으로 통일합니다.

`Engine/Bin/Shaders/` 폴더에 아래 내용으로 새 파일을 생성합니다.

**[추가] Engine/Bin/Shaders/Engine_Shader_Defines.hlsli**
```hlsl
// ---------------------------------------------------------
// 엔진 전역 셰이더 인클루드 (Engine_Shader_Defines.hlsli)
// ---------------------------------------------------------

// --- Transform Matrices ---
float4x4 g_WorldMatrix;
float4x4 g_ViewMatrix;
float4x4 g_ProjMatrix;

// --- Camera ---
vector g_CamPosition;

// --- Global Lighting ---
vector g_LightDir;
vector g_LightDiffuse;
vector g_LightAmbient;
vector g_LightSpecular;

// --- Material Default ---
// 주의: 기존 Shader_VtxMesh 등에 있던 g_MtrlAmbiment 오타를 g_MtrlAmbient로 수정 통일
vector g_MtrlAmbient = vector(0.3f, 0.3f, 0.3f, 1.f);
vector g_MtrlSpecular = vector(1.f, 1.f, 1.f, 1.f);

// --- Textures ---
Texture2D g_DiffuseTexture;
Texture2D g_MaskTexture;

// --- Common Sampler ---
sampler DefaultSampler = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = wrap;
    AddressV = wrap;
};

// ---------------------------------------------------------
// 엔진 전역 렌더링 상태 (Render States)
// ---------------------------------------------------------

// --- Rasterizer States ---
RasterizerState RS_Default
{
    FillMode = Solid;
    CullMode = Back;
};

RasterizerState RS_CullNone
{
    FillMode = Solid;
    CullMode = None;
};

RasterizerState RS_CullFront
{
    FillMode = Solid;
    CullMode = Front;
};

// --- Blend States ---
BlendState BS_Opaque
{
    BlendEnable[0] = False;
};

BlendState BS_AlphaBlend
{
    BlendEnable[0] = True;
    SrcBlend = Src_Alpha;
    DestBlend = Inv_Src_Alpha;
    BlendOp = Add;
};

// --- Depth Stencil States ---
DepthStencilState DSS_Default
{
    DepthEnable = True;
    DepthWriteMask = All;
};

DepthStencilState DSS_DepthDisable
{
    DepthEnable = False;
    DepthWriteMask = Zero;
};
```

### 1-2. `UpdateLib.bat`을 통한 복사 자동화 
루트 폴더에 있는 `UpdateLib.bat`을 수정하여, 엔진 빌드 시점에 `.hlsli`가 클라이언트로 복사되도록 구성합니다.

**[수정] UpdateLib.bat** 
기존 파일의 `popd` 위쪽에 셰이더 복사 스크립트를 추가합니다.
```bat
:: ... 기본 엔진 DLL/LIB 복사 구문들 ...
xcopy /y "Engine\Bin\Engine.lib" "EngineSDK\Lib\"
xcopy /y /s "Engine\Public\*.*" "EngineSDK\Include\"
xcopy /y "Server\Protobuf\Bin\*.pb.h" "EngineSDK\Include\"

:: [추가된 부분] 엔진 전역 셰이더 헤더를 각 프로젝트 셰이더 폴더로 복사
xcopy /y "Engine\Bin\Shaders\*.hlsli" "Client\Bin\Shaders\"
xcopy /y "Engine\Bin\Shaders\*.hlsli" "Editor\Bin\Shaders\"

popd
```

### 1-3. 모든 셰이더 파일 최상단 교체 및 통일
Client쪽에 있는 셰이더 파일 최상단의 중복 변수들을 지우고 `#include`문을 일괄 적용할 때, **반드시 기존 VS, PS 내부 로직의 변수명들도 통일된 이름으로 변경**해야 합니다.

**[예시] Client/Bin/Shaders/Shader_VtxMesh.hlsl 등**
```hlsl
// 1. 기존 전역 행렬, 라이트, 샘플러 선언들을 전부 삭제 후 대체
#include "Engine_Shader_Defines.hlsli"

struct VS_IN
{
    // ... 내용 유지 ...
};

// ... 수정해야 할 부분 (PS_MAIN) ...
// 기존 g_MtrlAmbiment -> g_MtrlAmbient 로 변경!
vector shade = saturate(max(dot(normalize(g_LightDir) * -1.f, In.vNormal), 0.f) + (g_LightAmbient * g_MtrlAmbient));
```

**[예시] Client/Bin/Shaders/Shader_VtxAnimMesh.hlsl 등**
```hlsl
// 1. 기존 변수 제거 & 글로벌 인클루드 (단, g_BoneMatrices 등 특수 변수는 남겨둠)
#include "Engine_Shader_Defines.hlsli"

row_major matrix g_BoneMatrices[512];

// ... 수정해야 할 부분 (본문 내 g_v 가 붙은 변수들을 떼어냄) ...
// 기존 g_vLightDir -> g_LightDir
// 기존 g_vCamPosition -> g_CamPosition
vector vShade = saturate(max(dot(normalize(g_LightDir) * -1.f, In.vNormal), 0.f) + (g_LightAmbient * g_MtrlAmbient));
```

---

## 2. 엔진 공용 구조체 및 Protobuf 추가

### [추가] Engine/Public/Engine_Struct.h
파티클 초기 생성에 필요한 DESC 구조체를 `Engine::` 네임스페이스 안에 작성힙니다. (원하는 위치에 추가)

```cpp
    // ---------------------------------------------------------
    // VtxParticle 방사 및 속성 Desc 설정
    // ---------------------------------------------------------
    typedef struct tagInstanceDesc
    {
        uint32  iNumInstances = {};  // 인스턴스 개수
        Vec3    vCenter;             // 생성 중심점
        Vec3    vRange;              // 중심에서 퍼지는 범위
        Vec2    vScale;              // 스케일 최소~최대 (랜덤)
    } INSTANCE_DESC;

    typedef struct tagParticleRectDesc : public tagInstanceDesc
    {
        Vec2    vSpeed;     // 떨어지는 최소~최대 속도 (x=min, y=max)
        Vec2    vLifeTime;  // 수명 최소~최대 시간
    } PARTICLE_RECT_DESC;

    typedef struct tagParticlePointDesc : public tagInstanceDesc
    {
        Vec2    vSpeed;     // 퍼지는 최소~최대 속도
        Vec2    vLifeTime;  // 수명 최소~최대 시간
        Vec3    vPivot;     // 폭발 발산 중심점
    } PARTICLE_POINT_DESC;
```

### [추가] Engine/Public/Vertex_Struct.h
인스턴스 버퍼로 전달할 요소들과 Input Layout 정보를 엔진 정점 헤더에 등록합니다. 기존 구조체들 아래에 전체 코드를 추가합니다.

```cpp
    // -------------------------------------------------------------
    // VtxParticleRect (Instance) : 사각형 파티클 인스턴싱용 데이터 
    // -------------------------------------------------------------
    typedef struct tagVertexParticleInstance
    {
        Vec4 vRight;        // 인스턴스별 X축 (스케일 포함)
        Vec4 vUp;           // 인스턴스별 Y축 (스케일 포함)
        Vec4 vLook;         // 인스턴스별 Z축 (스케일 포함)
        Vec4 vTranslation;  // 인스턴스별 위치
        Vec2 vLifeTime;     // x=최대수명, y=현재경과시간

        static const uint32 numElements = { 7 };
        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] = {
            // 슬롯 0 — 기본 정점 (Per-Vertex)
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },

            // 슬롯 1 — 인스턴스 데이터 (Per-Instance) -> TEXCOORD 시맨틱 사용
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
            { "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
            { "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
            { "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
            { "TEXCOORD", 5, DXGI_FORMAT_R32G32_FLOAT,       1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
        };
    } VTXPARTICLE_RECTINSTANCE;

    // -------------------------------------------------------------
    // VtxParticlePoint (Instance) : 점 파티클 (Geometry Shader 용)
    // -------------------------------------------------------------
    typedef struct tagVertexParticlePointInstance
    {
        // 정점 버퍼 구조는 사각형과 동일하지만, WORLD 시맨틱과 크기가 다른 정점 조합을 사용
        static const uint32 numElements = { 6 };
        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] = {
            // 슬롯 0 — 점 1개 (Per-Vertex)
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

            // 슬롯 1 — 인스턴스 4x4 행렬 + 수명 -> WORLD 시맨틱으로 받아 float4x4로 바인딩
            { "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
            { "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
            { "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
            { "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },  
        };
    } VTXPARTICLE_POINTINSTANCE;
```

### [추가/변경] Server/Protobuf/Protocol/Enum.proto
Protobuf ID를 추가 한 뒤, 반드시 **`GenProto.bat`**을 실행해야 엔진에서 사용할 수 있습니다.

```protobuf
// Protocol.ComponentID enum 안에 추가
COMPONENT_TYPE_SHADER_PARTICLE_RECT = 26;  // 비어있는 번호 할당
COMPONENT_TYPE_SHADER_PARTICLE_POINT = 27;
// ...
COMPONENT_TYPE_VIBUFFER_PARTICLE_RECT = 301;
COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT = 302;
```

---

## 3. 엔진 파티클 인스턴스 컴포넌트(VIBuffer) 생성 (완성본 코드)

`Engine/Private/` 와 `Engine/Public/` 에 총 6개의 파일을 새로 작성해야 합니다.

### 3-1. VIBuffer_Instance 기반 클래스

**[추가] Engine/Public/VIBuffer_Instance.h**
```cpp
#pragma once
#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer_Instance abstract : public VIBuffer
{
protected:
    explicit VIBuffer_Instance(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Instance(const VIBuffer_Instance& rhs);
    virtual ~VIBuffer_Instance() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual HRESULT Bind_Resources();
    virtual HRESULT Render();

protected:
    ComPtr<Buffer>      _instanceBuffer;     
    uint32              _instanceStride = 0; 
    uint32              _numInstances = 0;   
    uint32              _indexCountPerInstance = 0; 
    
    // CPU 상에서 매 프레임 업데이트할 기준 좌표들 배열 (Prototype이 소유)
    vector<Vec4>        _startPositions;     
    vector<float>       _speeds;
    bool                _isCloned = false;   

public:
    virtual Shared<Component> Clone(void* arg) = 0;
    virtual void Free() override;
};
NS_END
```

**[추가] Engine/Private/VIBuffer_Instance.cpp**
```cpp
#include "pch.h"
#include "VIBuffer_Instance.h"

VIBuffer_Instance::VIBuffer_Instance(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer(device, context) { }

VIBuffer_Instance::VIBuffer_Instance(const VIBuffer_Instance& rhs)
    : VIBuffer(rhs)
    , _instanceBuffer(rhs._instanceBuffer)
    , _instanceStride(rhs._instanceStride)
    , _numInstances(rhs._numInstances)
    , _indexCountPerInstance(rhs._indexCountPerInstance)
    , _startPositions(rhs._startPositions)
    , _speeds(rhs._speeds)
{
    _isCloned = true;
}

HRESULT VIBuffer_Instance::Initialize_Prototype() { return S_OK; }
HRESULT VIBuffer_Instance::Initialize(void* arg) { return S_OK; }

HRESULT VIBuffer_Instance::Bind_Resources()
{
    ComPtr<Buffer> vertexBuffers[] = { _vertexBuffer, _instanceBuffer };
    uint32 strides[] = { _vertexStride, _instanceStride };
    uint32 offsets[] = { 0, 0 };

    _context->IASetVertexBuffers(0, 2, vertexBuffers[0].GetAddressOf(), strides, offsets);
    
    if (_indexBuffer)
        _context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    _context->IASetPrimitiveTopology(_primitiveType);
    return S_OK;
}

HRESULT VIBuffer_Instance::Render()
{
    if (_indexBuffer) 
        _context->DrawIndexedInstanced(_indexCountPerInstance, _numInstances, 0, 0, 0);
    else 
        _context->DrawInstanced(1, _numInstances, 0, 0); 
    
    return S_OK;
}

void VIBuffer_Instance::Free()
{
    if (!_isCloned)
    {
        _startPositions.clear();
        _speeds.clear();
    }
    VIBuffer::Free();
}
```

### 3-2. VIBuffer_Particle_Rect 클래스 구현 (눈 파티클)

**[추가] Engine/Public/VIBuffer_Particle_Rect.h**
```cpp
#pragma once
#include "VIBuffer_Instance.h"

NS_BEGIN(Engine)
class ENGINE_DLL VIBuffer_Particle_Rect : public VIBuffer_Instance
{
    GENERATED_COMPONENT(VIBuffer_Particle_Rect, Protocol::COMPONENT_TYPE_VIBUFFER_PARTICLE_RECT)
public:
    explicit VIBuffer_Particle_Rect(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Particle_Rect(const VIBuffer_Particle_Rect& rhs);
    virtual ~VIBuffer_Particle_Rect() = default;

public:
    virtual HRESULT Initialize_Prototype(const PARTICLE_RECT_DESC* desc);
    virtual HRESULT Initialize(void* arg) override;
    void Drop(float timeDelta);

private:
    PARTICLE_RECT_DESC _desc = {};

public:
    static Shared<VIBuffer_Particle_Rect> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const PARTICLE_RECT_DESC* desc);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
};
NS_END
```

**[추가] Engine/Private/VIBuffer_Particle_Rect.cpp**
```cpp
#include "pch.h"
#include "VIBuffer_Particle_Rect.h"

VIBuffer_Particle_Rect::VIBuffer_Particle_Rect(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer_Instance(device, context) {}

VIBuffer_Particle_Rect::VIBuffer_Particle_Rect(const VIBuffer_Particle_Rect& rhs)
    : VIBuffer_Instance(rhs), _desc(rhs._desc) {}

HRESULT VIBuffer_Particle_Rect::Initialize_Prototype(const PARTICLE_RECT_DESC* desc)
{
    if (!desc) return E_FAIL;
    _desc = *desc;

    _numVertexBuffers = 2; // Basic Vertex + Instance Data
    _numVertices = 4;
    _vertexStride = sizeof(VTXTEX);
    _numIndices = 6;
    _indexStride = sizeof(uint16);
    _indexCountPerInstance = 6;
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    _numInstances = _desc.iNumInstances;
    _instanceStride = sizeof(VTXPARTICLE_RECTINSTANCE);

    // 1. Basic Rect Geometry
    vector<VTXTEX> vertices(_numVertices);
    vertices[0].position = Vec3(-0.5f,  0.5f, 0.f); vertices[0].texCoord = Vec2(0.f, 0.f);
    vertices[1].position = Vec3( 0.5f,  0.5f, 0.f); vertices[1].texCoord = Vec2(1.f, 0.f);
    vertices[2].position = Vec3( 0.5f, -0.5f, 0.f); vertices[2].texCoord = Vec2(1.f, 1.f);
    vertices[3].position = Vec3(-0.5f, -0.5f, 0.f); vertices[3].texCoord = Vec2(0.f, 1.f);

    D3D11_BUFFER_DESC vDesc{};
    vDesc.ByteWidth = _vertexStride * _numVertices;
    vDesc.Usage = D3D11_USAGE_DEFAULT;
    vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vData{};
    vData.pSysMem = vertices.data();
    _device->CreateBuffer(&vDesc, &vData, _vertexBuffer.GetAddressOf());

    // 2. Index Buffer
    vector<uint16> indices = { 0, 1, 2, 0, 2, 3 };
    D3D11_BUFFER_DESC iDesc{};
    iDesc.ByteWidth = _indexStride * _numIndices;
    iDesc.Usage = D3D11_USAGE_DEFAULT;
    iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iData{};
    iData.pSysMem = indices.data();
    _device->CreateBuffer(&iDesc, &iData, _indexBuffer.GetAddressOf());

    // 3. Instance 정점 계산 (Dynamic)
    vector<VTXPARTICLE_RECTINSTANCE> instanceData(_numInstances);
    _startPositions.resize(_numInstances);
    _speeds.resize(_numInstances);

    for (uint32 i = 0; i < _numInstances; ++i)
    {
        float size = Utils::RandomFloat(_desc.vScale.x, _desc.vScale.y);
        instanceData[i].vRight = Vec4(size, 0.f, 0.f, 0.f);
        instanceData[i].vUp    = Vec4(0.f, size, 0.f, 0.f);
        instanceData[i].vLook  = Vec4(0.f, 0.f, size, 0.f);
        
        float px = Utils::RandomFloat(_desc.vCenter.x - _desc.vRange.x, _desc.vCenter.x + _desc.vRange.x);
        float py = Utils::RandomFloat(_desc.vCenter.y - _desc.vRange.y, _desc.vCenter.y + _desc.vRange.y);
        float pz = Utils::RandomFloat(_desc.vCenter.z - _desc.vRange.z, _desc.vCenter.z + _desc.vRange.z);
        instanceData[i].vTranslation = Vec4(px, py, pz, 1.f);
        
        instanceData[i].vLifeTime.x = Utils::RandomFloat(_desc.vLifeTime.x, _desc.vLifeTime.y); // Max
        instanceData[i].vLifeTime.y = Utils::RandomFloat(0.f, instanceData[i].vLifeTime.x); // Current
        
        _startPositions[i] = instanceData[i].vTranslation;
        _speeds[i] = Utils::RandomFloat(_desc.vSpeed.x, _desc.vSpeed.y);
    }

    D3D11_BUFFER_DESC instDesc{};
    instDesc.ByteWidth = _instanceStride * _numInstances;
    instDesc.Usage = D3D11_USAGE_DYNAMIC;
    instDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    instDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA instData{};
    instData.pSysMem = instanceData.data();
    _device->CreateBuffer(&instDesc, &instData, _instanceBuffer.GetAddressOf());

    return S_OK;
}

HRESULT VIBuffer_Particle_Rect::Initialize(void* arg) { return S_OK; }

void VIBuffer_Particle_Rect::Drop(float timeDelta)
{
    D3D11_MAPPED_SUBRESOURCE mappedSub{};
    if (FAILED(_context->Map(_instanceBuffer.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedSub))) return;

    auto* pInstance = static_cast<VTXPARTICLE_RECTINSTANCE*>(mappedSub.pData);

    for (uint32 i = 0; i < _numInstances; ++i)
    {
        pInstance[i].vTranslation.y -= _speeds[i] * timeDelta;
        pInstance[i].vLifeTime.y += timeDelta;

        if (pInstance[i].vLifeTime.y >= pInstance[i].vLifeTime.x)
        {
            pInstance[i].vLifeTime.y = 0.f;
            pInstance[i].vTranslation = _startPositions[i];
        }
    }
    _context->Unmap(_instanceBuffer.Get(), 0);
}

Shared<VIBuffer_Particle_Rect> VIBuffer_Particle_Rect::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const PARTICLE_RECT_DESC* desc)
{
    auto instance = make_shared<VIBuffer_Particle_Rect>(device, context);
    if (FAILED(instance->Initialize_Prototype(desc))) return nullptr;
    return instance;
}

Shared<Component> VIBuffer_Particle_Rect::Clone(void* arg)
{
    auto instance = make_shared<VIBuffer_Particle_Rect>(*this);
    if (FAILED(instance->Initialize(arg))) return nullptr;
    return instance;
}

void VIBuffer_Particle_Rect::Free()
{
    VIBuffer_Instance::Free();
}
```


### 3-3. VIBuffer_Particle_Point 클래스 구현 (폭발용 점+GS 빌보드)

**[추가] Engine/Public/VIBuffer_Particle_Point.h**
```cpp
#pragma once
#include "VIBuffer_Instance.h"

NS_BEGIN(Engine)
class ENGINE_DLL VIBuffer_Particle_Point : public VIBuffer_Instance
{
    GENERATED_COMPONENT(VIBuffer_Particle_Point, Protocol::COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT)
public:
    explicit VIBuffer_Particle_Point(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Particle_Point(const VIBuffer_Particle_Point& rhs);
    virtual ~VIBuffer_Particle_Point() = default;

public:
    virtual HRESULT Initialize_Prototype(const PARTICLE_POINT_DESC* desc);
    virtual HRESULT Initialize(void* arg) override;
    void Spread(float timeDelta);

private:
    PARTICLE_POINT_DESC _desc = {};

public:
    static Shared<VIBuffer_Particle_Point> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const PARTICLE_POINT_DESC* desc);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
};
NS_END
```

**[추가] Engine/Private/VIBuffer_Particle_Point.cpp**
```cpp
#include "pch.h"
#include "VIBuffer_Particle_Point.h"

VIBuffer_Particle_Point::VIBuffer_Particle_Point(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer_Instance(device, context) {}

VIBuffer_Particle_Point::VIBuffer_Particle_Point(const VIBuffer_Particle_Point& rhs)
    : VIBuffer_Instance(rhs), _desc(rhs._desc) {}

HRESULT VIBuffer_Particle_Point::Initialize_Prototype(const PARTICLE_POINT_DESC* desc)
{
    if (!desc) return E_FAIL;
    _desc = *desc;

    _numVertexBuffers = 2; 
    _numVertices = 1;
    _vertexStride = sizeof(Vec3); // Position Only (VTXPOS 모방)
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; // Point List!
    _numInstances = _desc.iNumInstances;
    _instanceStride = sizeof(VTXPARTICLE_POINTINSTANCE);

    // 1. Basic Point Geometry
    Vec3 vertexPos = Vec3(0.f, 0.f, 0.f);

    D3D11_BUFFER_DESC vDesc{};
    vDesc.ByteWidth = _vertexStride * _numVertices;
    vDesc.Usage = D3D11_USAGE_DEFAULT;
    vDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vData{};
    vData.pSysMem = &vertexPos;
    _device->CreateBuffer(&vDesc, &vData, _vertexBuffer.GetAddressOf());

    // 인덱스 버퍼는 불필요함! _indexBuffer 널 포인트 유지

    // 2. Instance 정점 생성
    vector<VTXPARTICLE_POINTINSTANCE> instanceData(_numInstances);
    _startPositions.resize(_numInstances);
    _speeds.resize(_numInstances);

    for (uint32 i = 0; i < _numInstances; ++i)
    {
        float size = Utils::RandomFloat(_desc.vScale.x, _desc.vScale.y);
        
        // POINT 파티클은 사이즈를 4x4에 심어서 보냄. 셰이더에서는 Right, Up 길이로 사이즈 추정
        Vec4 right(size, 0.f, 0.f, 0.f);
        Vec4 up(0.f, size, 0.f, 0.f);
        Vec4 look(0.f, 0.f, size, 0.f);

        float px = Utils::RandomFloat(_desc.vCenter.x - _desc.vRange.x, _desc.vCenter.x + _desc.vRange.x);
        float py = Utils::RandomFloat(_desc.vCenter.y - _desc.vRange.y, _desc.vCenter.y + _desc.vRange.y);
        float pz = Utils::RandomFloat(_desc.vCenter.z - _desc.vRange.z, _desc.vCenter.z + _desc.vRange.z);
        Vec4 tran(px, py, pz, 1.f);
        
        instanceData[i].vRight = right;
        instanceData[i].vUp = up;
        instanceData[i].vLook = look;
        instanceData[i].vTranslation = tran;
        
        instanceData[i].vLifeTime.x = Utils::RandomFloat(_desc.vLifeTime.x, _desc.vLifeTime.y); 
        instanceData[i].vLifeTime.y = Utils::RandomFloat(0.f, instanceData[i].vLifeTime.x*0.5f); 
        
        _startPositions[i] = tran;
        _speeds[i] = Utils::RandomFloat(_desc.vSpeed.x, _desc.vSpeed.y);
    }

    D3D11_BUFFER_DESC instDesc{};
    instDesc.ByteWidth = _instanceStride * _numInstances;
    instDesc.Usage = D3D11_USAGE_DYNAMIC;
    instDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    instDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA instData{};
    instData.pSysMem = instanceData.data();
    _device->CreateBuffer(&instDesc, &instData, _instanceBuffer.GetAddressOf());

    return S_OK;
}

HRESULT VIBuffer_Particle_Point::Initialize(void* arg) { return S_OK; }

void VIBuffer_Particle_Point::Spread(float timeDelta)
{
    D3D11_MAPPED_SUBRESOURCE mappedSub{};
    if (FAILED(_context->Map(_instanceBuffer.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedSub))) return;

    auto* pInstance = static_cast<VTXPARTICLE_POINTINSTANCE*>(mappedSub.pData);

    for (uint32 i = 0; i < _numInstances; ++i)
    {
        Vec4 dirRaw = pInstance[i].vTranslation - Vec4(_desc.vPivot.x, _desc.vPivot.y, _desc.vPivot.z, 0.f);
        Vec3 dir(dirRaw.x, dirRaw.y, dirRaw.z);
        dir.Normalize();

        pInstance[i].vTranslation += Vec4(dir.x, dir.y, dir.z, 0.f) * _speeds[i] * timeDelta;
        pInstance[i].vLifeTime.y += timeDelta;

        if (pInstance[i].vLifeTime.y >= pInstance[i].vLifeTime.x)
        {
            pInstance[i].vLifeTime.y = 0.f;
            pInstance[i].vTranslation = _startPositions[i];
        }
    }
    _context->Unmap(_instanceBuffer.Get(), 0);
}

Shared<VIBuffer_Particle_Point> VIBuffer_Particle_Point::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const PARTICLE_POINT_DESC* desc)
{
    auto instance = make_shared<VIBuffer_Particle_Point>(device, context);
    if (FAILED(instance->Initialize_Prototype(desc))) return nullptr;
    return instance;
}

Shared<Component> VIBuffer_Particle_Point::Clone(void* arg)
{
    auto instance = make_shared<VIBuffer_Particle_Point>(*this);
    if (FAILED(instance->Initialize(arg))) return nullptr;
    return instance;
}

void VIBuffer_Particle_Point::Free()
{
    VIBuffer_Instance::Free();
}
```

---

## 4. 파티클 HLSL 구현체 작성 (앞서 생성한 .hlsli 인클루드)

### [추가] Client/Bin/Shaders/Shader_VtxParticleRect.hlsl
```hlsl
#include "Engine_Shader_Defines.hlsli"

struct VS_IN {
    float3 vPosition : POSITION;
    float2 vTexcoord : TEXCOORD0;
    
    // Instance Data
    float4 vRight : TEXCOORD1;
    float4 vUp : TEXCOORD2;
    float4 vLook : TEXCOORD3;
    float4 vTranslation : TEXCOORD4;
    float2 vLifeTime : TEXCOORD5;
};

struct VS_OUT {
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

VS_OUT VS_MAIN(VS_IN In) {
    VS_OUT Out;
    
    float4x4 TransformMatrix = float4x4(In.vRight, In.vUp, In.vLook, In.vTranslation);
    float4 vPosition = mul(float4(In.vPosition, 1.f), TransformMatrix);
    
    float4x4 matWV = mul(g_WorldMatrix, g_ViewMatrix);
    float4x4 matWVP = mul(matWV, g_ProjMatrix);
    
    Out.vPosition = mul(vPosition, matWVP);
    Out.vTexcoord = In.vTexcoord;
    Out.vLifeTime = In.vLifeTime;
    return Out;
}

struct PS_IN {
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

struct PS_OUT {
    vector vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In) {
    PS_OUT Out;
    Out.vColor = g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);
    
    if (Out.vColor.a < 0.1f) discard;
    
    Out.vColor.a *= saturate(In.vLifeTime.x - In.vLifeTime.y);
    return Out;
}

BlendState AlphaBlend { BlendEnable[0] = True; SrcBlend = Src_Alpha; DestBlend = Inv_Src_Alpha; BlendOp = Add; };
DepthStencilState DepthDisable { DepthEnable = false; DepthWriteMask = Zero; };

technique11 DefaultTechnique {
    pass DefaultPass {
        SetBlendState(AlphaBlend, float4(0.f,0.f,0.f,0.f), 0xffffffff);
        SetDepthStencilState(DepthDisable, 0);
        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}
```

### [추가] Client/Bin/Shaders/Shader_VtxParticlePoint.hlsl 
```hlsl
#include "Engine_Shader_Defines.hlsli"

struct VS_IN {
    float3 vPosition : POSITION;
    row_major float4x4 TransformMatrix : WORLD; 
    float2 vLifeTime : TEXCOORD0;
};

struct VS_OUT {
    float4 vPosition : SV_POSITION;
    float2 vPSize : PSIZE;
    float2 vLifeTime : TEXCOORD0;
};

VS_OUT VS_MAIN(VS_IN In) {
    VS_OUT Out;
    float4 vWorldPos = mul(float4(In.vPosition, 1.f), In.TransformMatrix);
    Out.vPosition = mul(vWorldPos, g_WorldMatrix);
    
    Out.vPSize = float2(
        length(float3(In.TransformMatrix._11, In.TransformMatrix._12, In.TransformMatrix._13)), 
        length(float3(In.TransformMatrix._21, In.TransformMatrix._22, In.TransformMatrix._23))
    );
    Out.vLifeTime = In.vLifeTime;
    return Out;
}

struct GS_IN {
    float4 vPosition : SV_POSITION;
    float2 vPSize : PSIZE;
    float2 vLifeTime : TEXCOORD0;
};

struct GS_OUT {
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

[maxvertexcount(6)]
void GS_MAIN(point GS_IN In[1], inout TriangleStream<GS_OUT> OutStream) {
    GS_OUT Out[4];
    
    float3 vLook = normalize(g_CamPosition.xyz - In[0].vPosition.xyz);
    float3 vRight = normalize(cross(float3(0.f, 1.f, 0.f), vLook)) * In[0].vPSize.x * 0.5f;
    float3 vUp = normalize(cross(vLook, vRight)) * In[0].vPSize.y * 0.5f;
    
    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);
    
    Out[0].vPosition = mul(vector(In[0].vPosition.xyz + vRight + vUp, 1.f), matVP);
    Out[0].vTexcoord = float2(0.f, 0.f);
    Out[0].vLifeTime = In[0].vLifeTime;
    
    Out[1].vPosition = mul(vector(In[0].vPosition.xyz + vRight - vUp, 1.f), matVP);
    Out[1].vTexcoord = float2(0.f, 1.f);
    Out[1].vLifeTime = In[0].vLifeTime;

    Out[2].vPosition = mul(vector(In[0].vPosition.xyz - vRight + vUp, 1.f), matVP);
    Out[2].vTexcoord = float2(1.f, 0.f);
    Out[2].vLifeTime = In[0].vLifeTime;

    Out[3].vPosition = mul(vector(In[0].vPosition.xyz - vRight - vUp, 1.f), matVP);
    Out[3].vTexcoord = float2(1.f, 1.f);
    Out[3].vLifeTime = In[0].vLifeTime;

    OutStream.Append(Out[0]);
    OutStream.Append(Out[1]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();
    
    OutStream.Append(Out[1]);
    OutStream.Append(Out[3]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();
}

struct PS_IN {
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

struct PS_OUT {
    vector vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In) {
    PS_OUT Out;
    Out.vColor = g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);
    if (Out.vColor.a < 0.1f) discard;
    
    Out.vColor.rgb += In.vLifeTime.y * 0.2f;
    Out.vColor.a *= saturate(In.vLifeTime.x - In.vLifeTime.y);

    return Out;
}

technique11 DefaultTechnique {
    pass DefaultPass {
        // 이미 hlsli에 선언된 BS_AlphaBlend, DSS_DepthDisable 사용 (동일하게 동작함)
        SetBlendState(BS_AlphaBlend, float4(0.f,0.f,0.f,0.f), 0xffffffff);
        SetDepthStencilState(DSS_DepthDisable, 0);
        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN(); // GS 사용
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}
```

---

## 5. 클라이언트 런타임 적용: ResourceLoader 데이터 로딩 

### [추가] Client/Bin/Resources/Data/json/DT_Shader.json 추가
```json
        {
            "Id": "COMPONENT_TYPE_SHADER_PARTICLE_RECT",
            "Path": "../../Client/Bin/Shaders/Shader_VtxParticleRect.hlsl",
            "Level": "Static",
            "Extra": "VtxParticleRect"
        },
        {
            "Id": "COMPONENT_TYPE_SHADER_PARTICLE_POINT",
            "Path": "../../Client/Bin/Shaders/Shader_VtxParticlePoint.hlsl",
            "Level": "Static",
            "Extra": "VtxParticlePoint"
        }
```

### [변경] Client/Private/ResourceLoader.cpp (완전 대체용 코드)
`Get_InputLayout` 함수 원본을 찾아 아래 코드로 완전히 교체합니다.

```cpp
ResourceLoader::FInputLayoutInfo ResourceLoader::Get_InputLayout(const string& name)
{
    if (name == "VtxTex")
        return { FVertexTex::Elements, FVertexTex::numElements };

    if (name == "VtxNorTex")
        return { FVertexNormalTex::Elements, FVertexNormalTex::numElements };

    if (name == "VtxMesh")
        return { FVertexMesh::Elements, FVertexMesh::numElements };

    if (name == "VtxAnim")
        return { FVertexAnimationMesh::Elements, FVertexAnimationMesh::numElements };

    if (name == "VtxParticleRect") // 추가
        return { VTXPARTICLE_RECTINSTANCE::Elements, VTXPARTICLE_RECTINSTANCE::numElements };

    if (name == "VtxParticlePoint") // 추가
        return { VTXPARTICLE_POINTINSTANCE::Elements, VTXPARTICLE_POINTINSTANCE::numElements };

    return { nullptr, 0 };
}
```

또한, 게임 어플리케이션 초기화 시에 Engine GameInstance에 VIBuffer Prototype들을 직접 생성해야 합니다. (`MainApp.cpp` 로딩 스텝)

```cpp
    // Prototype Manager에 수동으로 생성해주거나, 공용 Loader 로직 안에 삽입
    Engine::PARTICLE_RECT_DESC SnowDesc{};
    SnowDesc.iNumInstances = 5000;
    SnowDesc.vCenter = Vec3(0.f, 0.f, 0.f);
    SnowDesc.vRange = Vec3(129.f, 1.f, 129.f);
    SnowDesc.vScale = Vec2(0.2f, 0.5f);
    SnowDesc.vSpeed = Vec2(3.0f, 7.0f);   
    SnowDesc.vLifeTime = Vec2(3.f, 5.0f);   

    GAME->Add_Component_Prototype(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_VIBUFFER_PARTICLE_RECT,
        Engine::VIBuffer_Particle_Rect::Create(GAME->Get_Device(), GAME->Get_Context(), &SnowDesc));

    Engine::PARTICLE_POINT_DESC ExploDesc{};
    ExploDesc.iNumInstances = 500;
    ExploDesc.vCenter = Vec3(0.f, 0.f, 0.f);
    ExploDesc.vRange = Vec3(0.3f, 0.3f, 0.3f);
    ExploDesc.vScale = Vec2(0.2f, 0.5f);
    ExploDesc.vSpeed = Vec2(5.0f, 10.0f);   
    ExploDesc.vLifeTime = Vec2(1.f, 2.0f);
    ExploDesc.vPivot = Vec3(0.f, 0.f, 0.f);    

    GAME->Add_Component_Prototype(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT,
        Engine::VIBuffer_Particle_Point::Create(GAME->Get_Device(), GAME->Get_Context(), &ExploDesc));

```

---

## 6. GameObject 클라이언트 완성본

### [완성본 예시] CExplosion (폭발/포인트 파티클 사용 객체)
헤더 (`CExplosion.h`) 및 CPP (`CExplosion.cpp`) 예제입니다.

```cpp
// ---------------------------------------------
// 폭발 효과용 클라이언트 객체 (CExplosion.h)
// ---------------------------------------------
#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)
class Shader;
class Texture;
class VIBuffer_Particle_Point;
NS_END

class CExplosion : public GameObject
{
public:
    explicit CExplosion(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~CExplosion() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void Update(float timeDelta) override;
    virtual void Render() override;

private:
    Shared<Shader>                  _shader = nullptr;
    Shared<Texture>                 _texture = nullptr;
    Shared<VIBuffer_Particle_Point> _particleBuffer = nullptr;

public:
    static Shared<CExplosion> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
};
```

```cpp
// ---------------------------------------------
// 폭발 효과용 클라이언트 객체 (CExplosion.cpp)
// ---------------------------------------------
#include "pch.h"
#include "CExplosion.h"

CExplosion::CExplosion(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context) {}

HRESULT CExplosion::Initialize_Prototype() { return S_OK; }

HRESULT CExplosion::Initialize(void* arg)
{
    // 트랜스폼 등 공통 초기화
    GameObject::Initialize(arg);

    // 컴포넌트 Clone 바인딩 (프로토타입 팩토리 사용)
    _particleBuffer = dynamic_pointer_cast<VIBuffer_Particle_Point>(
        GAME->Clone_Component(Protocol::COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT));

    _shader = dynamic_pointer_cast<Shader>(
        GAME->Clone_Component(Protocol::COMPONENT_TYPE_SHADER_PARTICLE_POINT));

    // 폭발 효과로 사용할 텍스처 (예: Effect1)
    _texture = dynamic_pointer_cast<Texture>(
        GAME->Clone_Component(Get_ComponentID_From_String("Texture_Effect_Explode")));

    // 반투명 렌더링을 위해 블렌드 그룹 추가
    GAME->Add_RenderGroup(ERenderGroup::Blend, shared_from_this());

    return S_OK;
}

void CExplosion::Update(float timeDelta)
{
    // Spread 업데이트 (GPU 매핑) 수행
    if (_particleBuffer)
        _particleBuffer->Spread(timeDelta);
}

void CExplosion::Render()
{
    if (!_shader || !_particleBuffer || !_texture)
        return;

    // 1. 공통 매트릭스 바인딩 (Engine_Shader_Defines.hlsli 호환)
    _shader->Bind_Matrix("g_WorldMatrix", &_transform->Get_WorldMatrix());
    GAME->Bind_TransformMatrix(ETransformState::View, _shader, "g_ViewMatrix");
    GAME->Bind_TransformMatrix(ETransformState::Proj, _shader, "g_ProjMatrix");

    // 2. Geometry Shader가 요구하는 Camera Position
    GAME->Bind_CamPosition(_shader, "g_CamPosition");

    // 3. 텍스처
    _shader->Bind_SRV("g_DiffuseTexture", _texture->Get_SRV());

    // 4. Pass 시작 및 Render
    _shader->Begin_Pass(0);
    _particleBuffer->Bind_Resources();
    _particleBuffer->Render();
}

Shared<CExplosion> CExplosion::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<CExplosion>(device, context);
    if(FAILED(instance->Initialize_Prototype())) return nullptr;
    return instance;
}

Shared<GameObject> CExplosion::Clone(void* arg)
{
    auto instance = make_shared<CExplosion>(*this);
    if(FAILED(instance->Initialize(arg))) return nullptr;
    return instance;
}
```
* 위 예시는 `CSnow`의 경우 `VIBuffer_Particle_Rect` 컴포넌트로 변경 후 `Spread` 대신 `Drop`을 호출하면 바로 구현 가능합니다!
