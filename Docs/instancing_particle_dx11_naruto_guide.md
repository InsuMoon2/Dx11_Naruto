# Dx11_Naruto 인스턴싱 파티클 적용 가이드 A to Z

> 기준 자료
>
> - `D:\Jusin_158\9개월차\9개월차_18일차_19일차_Instancing_통합_리뷰.md`
> - `D:\Jusin_158\9개월차\20일차_Instancing2`
>
> 적용 대상
>
> - `d:\GitDesktop\Dx11_Naruto`

---

## 문서 목적

이 문서는 20일차 예제를 그대로 복붙하는 문서가 아니다.
이 문서는 **현재 `Dx11_Naruto` 구조에 맞게 Instancing Point Particle을 이식하는 설치 가이드**다.

이 문서의 목표는 아래 4가지다.

1. 18~20일차 예제의 핵심 개념을 현재 프로젝트 구조로 번역한다.
2. `DT_Shader.json`, `DT_Texture.json`, `DT_GameObject.json` 기반 데이터 흐름에 맞는 적용 순서를 제시한다.
3. 파일별 수정 위치와 새 파일 구조를 헤더/CPP/HLSL 수준까지 제시한다.
4. 1차 적용은 단일 SRV 기반으로 안정화하고, 2차 확장으로 다중 SRV / 프레임 애니메이션을 설명한다.

---

## 1. 먼저 결론

현재 프로젝트에 가장 잘 맞는 구조는 아래다.

- 엔진에 `VIBuffer_Instance`를 새로 만든다.
- 엔진에 `VIBuffer_Particle_Point`를 새로 만든다.
- `Vertex_Struct.h`에 Point Particle용 입력 레이아웃을 추가한다.
- `ResourceLoader::Get_InputLayout()`에 `VtxParticlePoint`를 추가한다.
- `Enum.proto`에 Shader / Texture / ObjectType / VIBuffer 관련 enum을 추가한다.
- 클라이언트에는 `InstancedParticle_Point` 오브젝트를 **하나만** 만든다.
- 이 오브젝트가 desc 값에 따라 `Drop`, `Spread`, `isLoop`를 바꿔 `Snow`와 `Explosion`을 공용 처리한다.
- 1차는 `Texture::Bind_SRV()`만 사용한다.
- 2차에서 `Shader::Bind_SRVs()`와 `Texture::Bind_SRVs()`를 추가해 프레임 애니메이션으로 확장한다.

즉, 20일차 예제처럼 `Snow.cpp`, `Explosion.cpp`를 각각 만드는 방식보다,
현재 프로젝트에는 **재사용형 파티클 오브젝트 1개 + 재사용형 버퍼 컴포넌트 1개**가 더 맞다.

---

## 2. 현재 프로젝트 기준 차이

| 구분 | 20일차 예제 | 현재 Dx11_Naruto | 적용 방식 |
|---|---|---|---|
| 정점 구조 | `Engine_Struct.h` | `Engine/Public/Vertex_Struct.h` | 여기에 새 struct 추가 |
| 버퍼 베이스 | `CVIBuffer_Instance` | 없음 | 새 파일 생성 |
| Point Particle Buffer | `CVIBuffer_Particle_Point` | 없음 | 새 파일 생성 |
| 셰이더 등록 | `Loader.cpp` 하드코딩 | `DT_Shader.json` + `ResourceLoader` | 데이터 방식으로 추가 |
| 텍스처 등록 | `Loader.cpp` 하드코딩 | `DT_Texture.json` | 데이터 방식으로 추가 |
| GameObject 등록 | 하드코딩 Prototype | `DT_GameObject.json` + `REGISTER_GAMEOBJECT` | enum + factory 추가 |
| 카메라 위치 바인딩 | 있음 | 이미 있음 | 그대로 사용 |
| 다중 SRV 바인딩 | 있음 | 없음 | 1차는 단일 SRV |

현재 프로젝트에서 중요한 사실은 아래 2개다.

1. `Texture`는 여러 SRV를 들고 있을 수 있지만, `Shader`는 현재 `Bind_SRV()`만 있다.
2. 즉, 20일차 예제의 `Bind_ShaderResourceViews()` 흐름은 현재 프로젝트에 그대로 이식되지 않는다.

그래서 1차 목표는 아래처럼 잡는 것이 안전하다.

- `Snow`: 단일 텍스처 + 루프형 + `Drop()`
- `Explosion`: 단일 텍스처 + 원샷형 + `Spread()`

---

## 3. 최종 파일 목록

### 수정 파일

- `d:\GitDesktop\Dx11_Naruto\Server\Protobuf\Protocol\Enum.proto`
- `d:\GitDesktop\Dx11_Naruto\Engine\Public\Vertex_Struct.h`
- `d:\GitDesktop\Dx11_Naruto\Client\Private\ResourceLoader.cpp`
- `d:\GitDesktop\Dx11_Naruto\Client\Private\Loader.cpp`
- `d:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\DT_Shader.json`
- `d:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\DT_Texture.json`
- `d:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\DT_GameObject.json`
- `d:\GitDesktop\Dx11_Naruto\Client\Private\Level_Gameplay.cpp`

### 새 파일

- `d:\GitDesktop\Dx11_Naruto\Engine\Public\VIBuffer_Instance.h`
- `d:\GitDesktop\Dx11_Naruto\Engine\Private\VIBuffer_Instance.cpp`
- `d:\GitDesktop\Dx11_Naruto\Engine\Public\VIBuffer_Particle_Point.h`
- `d:\GitDesktop\Dx11_Naruto\Engine\Private\VIBuffer_Particle_Point.cpp`
- `d:\GitDesktop\Dx11_Naruto\Client\Public\InstancedParticle_Point.h`
- `d:\GitDesktop\Dx11_Naruto\Client\Private\InstancedParticle_Point.cpp`
- `d:\GitDesktop\Dx11_Naruto\Client\Bin\Shaders\Shader_VtxParticlePoint.hlsl`

---

## 4. Step 1. Proto enum부터 추가

수정 파일:
`d:\GitDesktop\Dx11_Naruto\Server\Protobuf\Protocol\Enum.proto`

### [추가] 권장 enum

```proto
enum ComponentID
{
    // ... 기존 유지 ...

    COMPONENT_TYPE_SHADER_GHOST_EFFECT  = 407;
    COMPONENT_TYPE_SHADER_SMEAR_EFFECT  = 408;
    COMPONENT_TYPE_SHADER_VTXPARTICLE_POINT = 409; // [추가] Point Instancing Particle 셰이더

    COMPONENT_TYPE_TEXTURE_TARGET = 215;
    COMPONENT_TYPE_TEXTURE_PARTICLE_SNOW = 216;       // [추가] 눈 텍스처
    COMPONENT_TYPE_TEXTURE_PARTICLE_EXPLOSION = 217;  // [추가] 폭발 텍스처

    COMPONENT_TYPE_CLIENT_START = 1000;
    COMPONENT_TYPE_REPLICATOR   = 1001;
    // ... 기존 유지 ...
    COMPONENT_TYPE_SMEAR_EFFECT = 1013;
    COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT = 1014; // [추가] Point Particle Buffer
}

enum OBJECT_TYPE
{
    // ... 기존 유지 ...

    OBJECT_TYPE_SKILL_MONSTER_ATTACK = 40;
    OBJECT_TYPE_INSTANCED_PARTICLE_POINT = 41; // [추가] 공용 Point Particle Object
}
```

### [추가] 왜 필요한가

- `COMPONENT_TYPE_SHADER_VTXPARTICLE_POINT`: 셰이더 prototype 등록용
- `COMPONENT_TYPE_TEXTURE_PARTICLE_SNOW`: 눈 텍스처 등록용
- `COMPONENT_TYPE_TEXTURE_PARTICLE_EXPLOSION`: 폭발 텍스처 등록용
- `COMPONENT_TYPE_VIBUFFER_PARTICLE_POINT`: 버퍼 컴포넌트 factory clone용
- `OBJECT_TYPE_INSTANCED_PARTICLE_POINT`: 게임오브젝트 factory 등록용

### [추가] 재생성 절차

수정 후 반드시 아래를 실행한다.

1. `d:\GitDesktop\Dx11_Naruto\Server\Protobuf\Protocol\GenProto.bat`
2. 생성된 `Enum.pb.*` 반영 확인
3. `Engine/Public/Enum.pb.h`를 쓰는 경로가 최신인지 확인

---

## 5. Step 2. Point Particle 입력 레이아웃 추가

수정 파일:
`d:\GitDesktop\Dx11_Naruto\Engine\Public\Vertex_Struct.h`

### [추가] 코드

```cpp
namespace Engine
{
    typedef struct FVertexPos
    {
        Vec3 position; // [추가] Point Particle의 기본 정점 하나가 가지는 로컬 위치

        static const uint32 numElements = { 1 };

        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

    } VTXPOS;

    typedef struct FVertexParticleInstance
    {
        Vec4 vRight;       // [추가] 인스턴스별 Right 축과 X 스케일 역할
        Vec4 vUp;          // [추가] 인스턴스별 Up 축과 Y 스케일 역할
        Vec4 vLook;        // [추가] 인스턴스별 Look 축과 Z 스케일 역할
        Vec4 vTranslation; // [추가] 인스턴스별 월드 위치
        Vec2 vLifeTime;    // [추가] x=최대 수명, y=현재 누적 시간

    } VTXPARTICLE_INSTANCE;

    typedef struct FVertexParticlePointInstanceDesc
    {
        static const uint32 numElements = { 6 };

        static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },

            { "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        };

    } VTXPARTICLE_POINTINSTANCE_DESC;
}
```

### [추가] 해설

- `VTXPOS`: Point Particle은 기본 정점이 1개뿐이므로 필요
- `VTXPARTICLE_INSTANCE`: 인스턴스마다 위치, 축, 수명을 넘겨주는 구조
- `WORLD0~3`: HLSL에서 `row_major float4x4 TransformMatrix : WORLD`로 받기 위한 입력 시맨틱

---

## 6. Step 3. ResourceLoader에 새 레이아웃 이름 추가

수정 파일:
`d:\GitDesktop\Dx11_Naruto\Client\Private\ResourceLoader.cpp`

### [추가] 코드

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

    if (name == "VtxParticlePoint")
        return { VTXPARTICLE_POINTINSTANCE_DESC::Elements, VTXPARTICLE_POINTINSTANCE_DESC::numElements };

    return { nullptr, 0 };
}
```

---

## 7. Step 4. `VIBuffer_Instance` 추가

새 파일:
`d:\GitDesktop\Dx11_Naruto\Engine\Public\VIBuffer_Instance.h`

```cpp
#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer_Instance abstract : public VIBuffer
{
public:
    struct FInstanceDesc
    {
        uint32  numInstances = 0; // [추가] 생성할 인스턴스 개수
        Vec3    center = Vec3::Zero; // [추가] 생성 중심점
        Vec3    range = Vec3::Zero;  // [추가] 생성 범위
        Vec2    scale = Vec2(1.f, 1.f); // [추가] 랜덤 스케일 범위
    };

protected:
    explicit VIBuffer_Instance(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Instance(const VIBuffer_Instance& rhs);
    virtual ~VIBuffer_Instance();

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual HRESULT Bind_Resources();
    virtual HRESULT Render();

protected:
    ComPtr<Buffer>  _instanceBuffer;   // [추가] 인스턴스 전용 Vertex Buffer
    D3D11_BUFFER_DESC _instanceBufferDesc{};

    uint32 _instanceStride = 0;        // [추가] 인스턴스 1개 stride
    uint32 _numInstances = 0;          // [추가] 총 인스턴스 수
    uint32 _indexCountPerInstance = 0; // [추가] DrawIndexedInstanced용 인덱스 수

public:
    virtual Shared<Component> Clone(void* arg) override = 0;
    virtual void Free() override;
};

NS_END
```

새 파일:
`d:\GitDesktop\Dx11_Naruto\Engine\Private\VIBuffer_Instance.cpp`

```cpp
#include "pch.h"
#include "VIBuffer_Instance.h"

VIBuffer_Instance::VIBuffer_Instance(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer(device, context)
{
}

VIBuffer_Instance::VIBuffer_Instance(const VIBuffer_Instance& rhs)
    : VIBuffer(rhs)
    , _instanceBuffer(rhs._instanceBuffer)
    , _instanceBufferDesc(rhs._instanceBufferDesc)
    , _instanceStride(rhs._instanceStride)
    , _numInstances(rhs._numInstances)
    , _indexCountPerInstance(rhs._indexCountPerInstance)
{
}

VIBuffer_Instance::~VIBuffer_Instance()
{
}

HRESULT VIBuffer_Instance::Initialize_Prototype()
{
    return S_OK;
}

HRESULT VIBuffer_Instance::Initialize(void* arg)
{
    return S_OK;
}

HRESULT VIBuffer_Instance::Bind_Resources()
{
    ID3D11Buffer* vertexBuffers[] =
    {
        _vertexBuffer.Get(),
        _instanceBuffer.Get(),
    };

    uint32 strides[] =
    {
        _vertexStride,
        _instanceStride,
    };

    uint32 offsets[] = { 0, 0 };

    _context->IASetVertexBuffers(0, _numVertexBuffers, vertexBuffers, strides, offsets);
    _context->IASetIndexBuffer(_indexBuffer.Get(), _indexStride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    _context->IASetPrimitiveTopology(_primitiveType);

    return S_OK;
}

HRESULT VIBuffer_Instance::Render()
{
    _context->DrawIndexedInstanced(_indexCountPerInstance, _numInstances, 0, 0, 0);
    return S_OK;
}

void VIBuffer_Instance::Free()
{
    VIBuffer::Free();
}
```

---

## 8. Step 5. `VIBuffer_Particle_Point` 추가

새 파일:
`d:\GitDesktop\Dx11_Naruto\Engine\Public\VIBuffer_Particle_Point.h`

```cpp
#pragma once

#include "VIBuffer_Instance.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer_Particle_Point final : public VIBuffer_Instance
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
        Vec3    pivot = Vec3::Zero;       // [추가] Spread의 중심점
        Vec2    speed = Vec2(1.f, 1.f);   // [추가] 랜덤 속도 범위
        Vec2    lifeTime = Vec2(1.f, 1.f); // [추가] 랜덤 수명 범위
        bool    isLoop = false;           // [추가] 수명 종료 후 재생성 여부
        EMoveMode moveMode = EMoveMode::Drop; // [추가] 이동 방식
    };

public:
    explicit VIBuffer_Particle_Point(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer_Particle_Point(const VIBuffer_Particle_Point& rhs);
    virtual ~VIBuffer_Particle_Point();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    HRESULT Bind_Resources() override;
    HRESULT Render() override;

    void Update_Particles(float timeDelta); // [추가] 외부 오브젝트가 매 프레임 호출하는 업데이트 함수

private:
    void Update_Drop(float timeDelta);   // [추가] 눈처럼 아래로 떨어지는 이동 처리
    void Update_Spread(float timeDelta); // [추가] 중심에서 바깥으로 퍼지는 이동 처리

private:
    vector<VTXPARTICLE_INSTANCE> _initialInstances; // [추가] 루프형 리셋 기준이 되는 CPU 원본
    vector<float>                _speeds;           // [추가] 인스턴스별 랜덤 속도

    Vec3        _pivot = Vec3::Zero;                // [추가] Spread 중심점
    bool        _isLoop = false;                    // [추가] 루프형 여부
    EMoveMode   _moveMode = EMoveMode::Drop;        // [추가] 현재 이동 정책

public:
    static Shared<VIBuffer_Particle_Point> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### [추가] 구현 핵심 포인트

- `Initialize_Prototype()`에서는 기본 정점 1개짜리 PointList 버퍼를 만든다.
- `Initialize(void* arg)`에서는 전달받은 `FParticlePointDesc`로 인스턴스 버퍼를 실제 생성한다.
- `Update_Particles()`는 `moveMode`를 보고 `Update_Drop()` 또는 `Update_Spread()`를 부른다.
- 렌더링은 `DrawInstanced(1, _numInstances, 0, 0)`를 사용한다.

### [추가] 꼭 지켜야 할 구현 규칙

- 새 멤버마다 역할 주석을 붙인다.
- 새 함수마다 왜 필요한지, 언제 호출되는지 주석을 붙인다.
- `Map/Unmap`으로 인스턴스 버퍼를 직접 갱신한다.
- 루프형은 `_initialInstances` 기준으로 위치와 시간을 되돌린다.

---

## 9. Step 6. 셰이더 추가

새 파일:
`d:\GitDesktop\Dx11_Naruto\Client\Bin\Shaders\Shader_VtxParticlePoint.hlsl`

### [추가] 1차 안정 버전 셰이더

```hlsl
#include "Engine_Shader_Defines.hlsli"

texture2D g_DiffuseTexture;
sampler DefaultSampler = sampler_state
{
    Filter = MIN_MAG_MIP_LINEAR;
};

struct VS_IN
{
    float3 vPosition : POSITION;
    row_major float4x4 TransformMatrix : WORLD;
    float2 vLifeTime : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : POSITION;
    float2 vPSize : PSIZE;
    float2 vLifeTime : TEXCOORD0;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    vector vPosition = mul(float4(In.vPosition, 1.f), In.TransformMatrix);
    Out.vPosition = mul(vPosition, g_WorldMatrix);
    Out.vPSize = float2(length(In.TransformMatrix._11_12_13), length(In.TransformMatrix._21_22_23));
    Out.vLifeTime = In.vLifeTime;

    return Out;
}

struct GS_IN
{
    float4 vPosition : POSITION;
    float2 vPSize : PSIZE;
    float2 vLifeTime : TEXCOORD0;
};

struct GS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

[maxvertexcount(6)]
void GS_MAIN(point GS_IN In[1], inout TriangleStream<GS_OUT> OutStream)
{
    GS_OUT Out[4];

    float3 vLook = g_vCamPosition.xyz - In[0].vPosition.xyz;
    float3 vRight = normalize(cross(float3(0.f, 1.f, 0.f), vLook)) * In[0].vPSize.x * 0.5f;
    float3 vUp = normalize(cross(vLook, vRight)) * In[0].vPSize.y * 0.5f;

    matrix matVP = mul(g_ViewMatrix, g_ProjMatrix);

    Out[0].vPosition = mul(vector(In[0].vPosition.xyz + vRight + vUp, 1.f), matVP);
    Out[0].vTexcoord = float2(0.f, 0.f);
    Out[0].vLifeTime = In[0].vLifeTime;

    Out[1].vPosition = mul(vector(In[0].vPosition.xyz - vRight + vUp, 1.f), matVP);
    Out[1].vTexcoord = float2(1.f, 0.f);
    Out[1].vLifeTime = In[0].vLifeTime;

    Out[2].vPosition = mul(vector(In[0].vPosition.xyz - vRight - vUp, 1.f), matVP);
    Out[2].vTexcoord = float2(1.f, 1.f);
    Out[2].vLifeTime = In[0].vLifeTime;

    Out[3].vPosition = mul(vector(In[0].vPosition.xyz + vRight - vUp, 1.f), matVP);
    Out[3].vTexcoord = float2(0.f, 1.f);
    Out[3].vLifeTime = In[0].vLifeTime;

    OutStream.Append(Out[0]);
    OutStream.Append(Out[1]);
    OutStream.Append(Out[2]);
    OutStream.RestartStrip();

    OutStream.Append(Out[0]);
    OutStream.Append(Out[2]);
    OutStream.Append(Out[3]);
    OutStream.RestartStrip();
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    Out.vColor = g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord);

    if (Out.vColor.a < 0.3f)
        discard;

    Out.vColor.rgb *= saturate(1.f - (In.vLifeTime.y / max(In.vLifeTime.x, 0.001f)));
    Out.vColor.a *= saturate(In.vLifeTime.x - In.vLifeTime.y);

    return Out;
}

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_ZTest_NoWrite, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = compile gs_5_0 GS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}
```

### [추가] 중요한 포인트

- 현재 프로젝트에는 `g_vCamPosition`을 이미 바인딩하는 코드가 여러 군데 있다.
- 이 셰이더는 반드시 `GAME->Bind_CamPosition(_shaderCom, "g_vCamPosition")`와 같이 연결되어야 한다.
- 1차는 `texture2D g_DiffuseTexture` 하나만 쓴다.

---

## 10. Step 7. 클라이언트 오브젝트 `InstancedParticle_Point` 추가

새 파일:
`d:\GitDesktop\Dx11_Naruto\Client\Public\InstancedParticle_Point.h`

```cpp
#pragma once

#include "GameObject.h"
#include "VIBuffer_Particle_Point.h"

NS_BEGIN(Engine)
class Shader;
class Texture;
NS_END

NS_BEGIN(Client)

class InstancedParticle_Point final : public GameObject
{
    GENERATED_BODY(InstancedParticle_Point)

public:
    struct FInstancedParticlePointDesc : public GameObject::FGameObjectDesc
    {
        uint32 shaderType = Protocol::COMPONENT_TYPE_SHADER_VTXPARTICLE_POINT; // [추가] 사용할 셰이더 컴포넌트 ID
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_PARTICLE_SNOW;    // [추가] 사용할 텍스처 컴포넌트 ID
        uint32 textureIndex = 0;                                                // [추가] 멀티 SRV일 때 사용할 텍스처 인덱스

        VIBuffer_Particle_Point::FParticlePointDesc bufferDesc{};               // [추가] 파티클 버퍼 생성에 필요한 전체 설정값
        bool addToBlendGroup = true;                                            // [추가] 투명 파티클이므로 기본값은 Blend
    };

public:
    explicit InstancedParticle_Point(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit InstancedParticle_Point(const InstancedParticle_Point& rhs);
    virtual ~InstancedParticle_Point();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void Priority_Update(float timeDelta) override;
    void Update(float timeDelta) override;
    void Late_Update(float timeDelta) override;
    HRESULT Render() override;
    HRESULT Bind_ShaderResources() override;

private:
    HRESULT Ready_Components(const FInstancedParticlePointDesc& desc); // [추가] Shader/Texture/VIBuffer를 desc 기준으로 조립

private:
    Shared<Shader> _shaderCom;
    Shared<Texture> _textureCom;
    Shared<VIBuffer_Particle_Point> _bufferCom;

    uint32 _textureIndex = 0;   // [추가] 단일 SRV 기준이면 0, 이후 확장 대비 보관
    bool _addToBlendGroup = true; // [추가] 렌더 그룹 정책

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### [추가] 구현 흐름

- `Initialize()`에서 desc를 받아 `Ready_Components(desc)` 호출
- `Ready_Components()`에서
  - Shader clone
  - Texture clone
  - `VIBuffer_Particle_Point` clone with `bufferDesc`
- `Update()`에서 `_bufferCom->Update_Particles(timeDelta)` 호출
- `Late_Update()`에서 `ERenderGroup::Blend`에 넣기
- `Render()`에서 shader begin -> buffer bind -> draw

### [추가] 렌더 핵심

```cpp
HRESULT InstancedParticle_Point::Bind_ShaderResources()
{
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_DiffuseTexture", _textureIndex), E_FAIL);
    CHECK_FAILED(GAME->Bind_CamPosition(_shaderCom, "g_vCamPosition"), E_FAIL);

    return S_OK;
}
```

---

## 11. Step 8. Loader에 factory 등록 추가

수정 파일:
`d:\GitDesktop\Dx11_Naruto\Client\Private\Loader.cpp`

### [추가] include

```cpp
#include "VIBuffer_Particle_Point.h"
#include "InstancedParticle_Point.h"
```

### [추가] Register_Components()

```cpp
GAME->Register_ComponentFactory<VIBuffer_Particle_Point>(staticLevel);
```

### [추가] GameObject factory

`InstancedParticle_Point.cpp` 상단에 아래를 추가한다.

```cpp
REGISTER_GAMEOBJECT(InstancedParticle_Point, Protocol::OBJECT_TYPE_INSTANCED_PARTICLE_POINT)
```

이렇게 해야 `DT_GameObject.json`에 넣은 `OBJECT_TYPE_INSTANCED_PARTICLE_POINT`가 실제 클래스와 연결된다.

---

## 12. Step 9. 데이터 테이블 등록

### [추가] `DT_Shader.json`

수정 파일:
`d:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\DT_Shader.json`

```json
{
    "Id": "COMPONENT_TYPE_SHADER_VTXPARTICLE_POINT",
    "Path": "../../Client/Bin/Shaders/Shader_VtxParticlePoint.hlsl",
    "Level": "Static",
    "Extra": "VtxParticlePoint"
}
```

### [추가] `DT_Texture.json`

수정 파일:
`d:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\DT_Texture.json`

```json
{
    "Id": "COMPONENT_TYPE_TEXTURE_PARTICLE_SNOW",
    "Path": "../../Client/Bin/Resources/Textures/Snow/Snow.png",
    "Count": 1,
    "Level": "Static"
}
```

```json
{
    "Id": "COMPONENT_TYPE_TEXTURE_PARTICLE_EXPLOSION",
    "Path": "../../Client/Bin/Resources/Textures/Explosion/Explosion0.png",
    "Count": 1,
    "Level": "Static"
}
```

### [추가] `DT_GameObject.json`

수정 파일:
`d:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\DT_GameObject.json`

```json
{
    "Id": "InstancedParticlePoint",
    "ObjectType": "OBJECT_TYPE_INSTANCED_PARTICLE_POINT",
    "Level": "Static"
}
```

---

## 13. Step 10. Level에서 먼저 수동 생성으로 붙여라

수정 파일:
`d:\GitDesktop\Dx11_Naruto\Client\Private\Level_Gameplay.cpp`

### [추가] Snow 생성 예시

```cpp
InstancedParticle_Point::FInstancedParticlePointDesc snowDesc{};
snowDesc.name = L"Particle_Snow";
snowDesc.position = Vec3(64.f, 30.f, 64.f);
snowDesc.shaderType = Protocol::COMPONENT_TYPE_SHADER_VTXPARTICLE_POINT;
snowDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_PARTICLE_SNOW;
snowDesc.textureIndex = 0;
snowDesc.bufferDesc.numInstances = 5000;
snowDesc.bufferDesc.center = Vec3::Zero;
snowDesc.bufferDesc.range = Vec3(129.f, 1.f, 129.f);
snowDesc.bufferDesc.scale = Vec2(0.2f, 0.5f);
snowDesc.bufferDesc.speed = Vec2(3.f, 7.f);
snowDesc.bufferDesc.lifeTime = Vec2(3.f, 5.f);
snowDesc.bufferDesc.isLoop = true;
snowDesc.bufferDesc.moveMode = VIBuffer_Particle_Point::EMoveMode::Drop;

CHECK_FAILED(
    GAME->Add_GameObject(
        ETOI(ELevelType::GamePlay),
        Protocol::OBJECT_TYPE_INSTANCED_PARTICLE_POINT,
        TEXT("Layer_Effect"),
        &snowDesc),
    E_FAIL);
```

### [추가] Explosion 생성 예시

```cpp
InstancedParticle_Point::FInstancedParticlePointDesc explosionDesc{};
explosionDesc.name = L"Particle_Explosion";
explosionDesc.position = Vec3(0.f, 2.f, 0.f);
explosionDesc.shaderType = Protocol::COMPONENT_TYPE_SHADER_VTXPARTICLE_POINT;
explosionDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_PARTICLE_EXPLOSION;
explosionDesc.textureIndex = 0;
explosionDesc.bufferDesc.numInstances = 500;
explosionDesc.bufferDesc.center = Vec3::Zero;
explosionDesc.bufferDesc.range = Vec3(0.3f, 0.3f, 0.3f);
explosionDesc.bufferDesc.scale = Vec2(0.1f, 0.2f);
explosionDesc.bufferDesc.speed = Vec2(3.f, 7.f);
explosionDesc.bufferDesc.lifeTime = Vec2(1.f, 2.f);
explosionDesc.bufferDesc.pivot = Vec3::Zero;
explosionDesc.bufferDesc.isLoop = false;
explosionDesc.bufferDesc.moveMode = VIBuffer_Particle_Point::EMoveMode::Spread;

CHECK_FAILED(
    GAME->Add_GameObject(
        ETOI(ELevelType::GamePlay),
        Protocol::OBJECT_TYPE_INSTANCED_PARTICLE_POINT,
        TEXT("Layer_Effect"),
        &explosionDesc),
    E_FAIL);
```

### [추가] 클릭 테스트 예시

```cpp
if (INPUT->KeyDown(KEY_TYPE::LBTN))
{
    auto clickExplosion = explosionDesc;
    clickExplosion.position = Vec3(0.f, 1.f, 0.f);

    GAME->Add_GameObject(
        ETOI(ELevelType::GamePlay),
        Protocol::OBJECT_TYPE_INSTANCED_PARTICLE_POINT,
        TEXT("Layer_Effect"),
        &clickExplosion);
}
```

---

## 14. Step 11. 테스트 순서

적용 후 확인은 아래 순서로 한다.

1. 빌드가 되는지 확인
2. `DT_Shader.json`에서 새 셰이더가 등록되는지 확인
3. `DT_Texture.json`에서 Snow/Explosion 텍스처가 등록되는지 확인
4. `DT_GameObject.json`에서 새 object type이 factory로 생성되는지 확인
5. 레벨 진입 시 Snow가 보이는지 확인
6. 클릭 시 Explosion이 퍼지는지 확인
7. DrawCall이 개별 파티클 수만큼 늘지 않는지 확인

### [추가] 디버깅 포인트

- 안 보이면:
  - 셰이더 path
  - input layout 이름
  - `g_vCamPosition` 바인딩
  - Blend 그룹 추가 여부
  - 텍스처 alpha discard
- 터지면:
  - enum 재생성 여부
  - `VTXPARTICLE_POINTINSTANCE_DESC` stride/offset
  - `DrawInstanced` 사용 여부

---

## 15. 2차 확장: Explosion 프레임 애니메이션

현재 프로젝트는 `Shader::Bind_SRV()`만 있다.
그래서 20일차 예제의 다중 SRV 바인딩을 그대로 가져올 수 없다.

### [변경] 2차에서 해야 할 것

- `Shader.h/.cpp`에 `Bind_SRVs(const char* constantName, ID3D11ShaderResourceView* const* srvs, uint32 count)` 추가
- `Texture`에 `Bind_SRVs(Shared<Shader> shader, const char* constantName)` 추가
- `InstancedParticle_Point`에 frame index 또는 elapsed time 기반 텍스처 선택 정책 추가
- `DT_Texture.json`에서 `Explosion%d.png`를 `Count = 28`로 등록

### [추가] 지금 당장 추천하지 않는 이유

현재 프로젝트에 인스턴싱 파티클 자체가 아직 없으므로,
처음부터 다중 SRV + 프레임 애니메이션까지 넣으면 디버깅 지점이 너무 많아진다.

그래서 순서는 반드시 아래처럼 간다.

1. Point Instancing Particle이 보이게 만들기
2. `Drop()` / `Spread()` 동작 검증
3. 그 다음 텍스처 확장

---

## 16. 최종 체크리스트

- `Enum.proto`에 Shader / Texture / ObjectType / VIBuffer enum 추가 완료
- protobuf 재생성 완료
- `Vertex_Struct.h`에 `VTXPOS`, `VTXPARTICLE_INSTANCE`, `VTXPARTICLE_POINTINSTANCE_DESC` 추가 완료
- `ResourceLoader::Get_InputLayout()`에 `VtxParticlePoint` 추가 완료
- `VIBuffer_Instance` 추가 완료
- `VIBuffer_Particle_Point` 추가 완료
- `Shader_VtxParticlePoint.hlsl` 추가 완료
- `InstancedParticle_Point` 추가 완료
- `Loader::Register_Components()`에 `VIBuffer_Particle_Point` 등록 완료
- `DT_Shader.json`, `DT_Texture.json`, `DT_GameObject.json` 반영 완료
- `Level_Gameplay.cpp`에서 Snow / Explosion 생성 확인 완료

---

## 17. 이 문서 기준 최종 권장 순서

진짜 작업 순서는 아래 순서를 지키는 것이 가장 안전하다.

1. `Enum.proto` 수정
2. protobuf 재생성
3. `Vertex_Struct.h` 수정
4. `ResourceLoader::Get_InputLayout()` 수정
5. `VIBuffer_Instance` 생성
6. `VIBuffer_Particle_Point` 생성
7. `Shader_VtxParticlePoint.hlsl` 생성
8. `InstancedParticle_Point` 생성
9. `Loader::Register_Components()` 수정
10. `DT_Shader.json` 수정
11. `DT_Texture.json` 수정
12. `DT_GameObject.json` 수정
13. `Level_Gameplay.cpp`에서 직접 스폰
14. 보이면 그 다음 prefab화
15. 안정화 후 다중 SRV 확장

이 순서를 어기면 보통 어디선가 “보이지는 않는데 에러도 없는 상태”에 빠진다.
특히 `Enum`, `InputLayout`, `Shader`, `GameObject Factory` 4개는 하나라도 빠지면 디버깅이 길어진다.

이 문서대로 1차 적용이 끝나면,
그 다음 단계는 `Snow.prefab.json`, `Explosion.prefab.json`로 승격하고
`Spawn_Helper::Prefab()` 흐름에 얹는 것이다.
