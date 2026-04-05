#pragma once

#include "Base.h"
#include <DirectXTK/Effects.h>
#include <DirectXTK/PrimitiveBatch.h>
#include <DirectXTK/VertexTypes.h>

NS_BEGIN(Engine)

struct ENGINE_DLL FDebugRenderStyle
{
    Color color = Color(1.f, 1.f, 1.f, 1.f); // 디버그 도형의 렌더 색상
    float duration = 0.f;                    // 0 이하면 1프레임, 양수면 유지 시간(초)
    bool depthEnabled = true;                // true면 depth test 적용, false면 overlay처럼 출력
};

struct ENGINE_DLL FDebugBoxDesc
{
    Vec3 center = Vec3::Zero;                // 월드 기준 박스 중심 위치
    Vec3 extents = Vec3(0.5f, 0.5f, 0.5f);   // 월드 기준 박스 반크기
    Quat rotation = Quat::Identity;          // 월드 기준 박스 회전
    FDebugRenderStyle style;                 // 박스 공통 렌더 옵션
};

struct ENGINE_DLL FDebugSphereDesc
{
    Vec3 center = Vec3::Zero;                // 월드 기준 구 중심 위치
    float radius = 0.5f;                     // 월드 기준 구 반지름
    FDebugRenderStyle style;                 // 구 공통 렌더 옵션
};

struct ENGINE_DLL FDebugLineDesc
{
    Vec3 start = Vec3::Zero;                 // 월드 기준 선 시작점
    Vec3 end = Vec3::Zero;                   // 월드 기준 선 끝점
    FDebugRenderStyle style;                 // 선 공통 렌더 옵션
};

struct ENGINE_DLL FDebugMeshDesc
{
    Shared<class Model> model;              
    Matrix worldMatrix = Matrix::Identity;  
    FDebugRenderStyle style;                
};

class ENGINE_DLL Debug_Manager final : public Base
{
public:
    explicit Debug_Manager(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Debug_Manager() = default;

public:
    // PrimitiveBatch / BasicEffect / InputLayout을 1회 생성할 때 호출한다.
    HRESULT Initialize();

    // 프레임 시작 시 이전 1프레임 요청을 제거하고 duration 기반 요청의 남은 시간을 줄일 때 호출한다.
    void Tick(float timeDelta);

    // depth test가 켜진 디버그 도형 요청을 렌더러가 처리할 때 호출한다.
    HRESULT Render_Depth();

    // depth test가 꺼진 overlay 성격의 디버그 도형 요청을 렌더러가 처리할 때 호출한다.
    HRESULT Render_Overlay();

public:
    // 월드 기준 디버그 박스를 큐에 등록할 때 호출한다.
    void Draw_Box(const FDebugBoxDesc& desc);

    // 월드 기준 디버그 구를 큐에 등록할 때 호출한다.
    void Draw_Sphere(const FDebugSphereDesc& desc);

    // 월드 기준 디버그 선을 큐에 등록할 때 호출한다.
    void Draw_Line(const FDebugLineDesc& desc);

    void Draw_Mesh(const FDebugMeshDesc& desc);

public:
    // 누적된 모든 디버그 요청을 즉시 비울 때 호출한다.
    void Clear();

    // 전체 디버그 렌더 출력을 켜고 끌 때 호출한다.
    void Set_Enabled(bool enabled) { _isEnabled = enabled; }

    // 현재 디버그 렌더 출력 가능 여부를 조회할 때 호출한다.
    bool Is_Enabled() const { return _isEnabled; }

private:
    struct FDebugLifetime
    {
        float remainingTime = 0.f; // duration 기반 요청의 남은 시간
        bool isOneFrame = true;    // true면 다음 Tick에서 제거되는 1프레임 요청
    };

    struct FDebugBoxEntry
    {
        BoundingOrientedBox box;               // 실제 렌더에 사용할 박스 정보
        Color color = Color(1.f, 1.f, 1.f, 1.f); // 박스 렌더 색상
        FDebugLifetime lifetime;               // 박스 유지 시간 정보
    };

    struct FDebugSphereEntry
    {
        BoundingSphere sphere;                 // 실제 렌더에 사용할 구 정보
        Color color = Color(1.f, 1.f, 1.f, 1.f); // 구 렌더 색상
        FDebugLifetime lifetime;               // 구 유지 시간 정보
    };

    struct FDebugLineEntry
    {
        Vec3 start = Vec3::Zero;               // 선 시작점
        Vec3 end = Vec3::Zero;                 // 선 끝점
        Color color = Color(1.f, 1.f, 1.f, 1.f); // 선 렌더 색상
        FDebugLifetime lifetime;               // 선 유지 시간 정보
    };

    struct FDebugMeshEntry
    {
        Shared<Model> model;
        Matrix worldMatrix;
        Color color = Color(1.f, 1.f, 1.f, 1.f);
        FDebugLifetime lifetime;
    };

private:
    // BasicEffect의 view/proj와 input layout을 바인딩하기 직전에 호출한다.
    HRESULT Prepare_RenderState();

    // 박스 큐를 실제 draw call로 풀어낼 때 호출한다.
    void Render_BoxEntries(const vector<FDebugBoxEntry>& entries);

    // 구 큐를 실제 draw call로 풀어낼 때 호출한다.
    void Render_SphereEntries(const vector<FDebugSphereEntry>& entries);

    // 선 큐를 실제 draw call로 풀어낼 때 호출한다.
    void Render_LineEntries(const vector<FDebugLineEntry>& entries);

    void Render_MeshEntries(const vector<FDebugMeshEntry>& entries);

private:
    ComPtr<Device> _device;                   
    ComPtr<DeviceContext> _context;           

    Shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> _batch; 
    Shared<DirectX::BasicEffect> _effect;     
    ComPtr<ID3D11InputLayout> _inputLayout;   

    vector<FDebugBoxEntry> _depthBoxes;       
    vector<FDebugSphereEntry> _depthSpheres;  
    vector<FDebugLineEntry> _depthLines;      

    vector<FDebugBoxEntry> _overlayBoxes;     
    vector<FDebugSphereEntry> _overlaySpheres;
    vector<FDebugLineEntry> _overlayLines;    

    vector<FDebugMeshEntry> _depthMeshes;
    vector<FDebugMeshEntry> _overlayMeshes;

    bool _isEnabled = true;                    // 전체 디버그 렌더 출력을 허용할지 여부

public:
    static Unique<Debug_Manager> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    void Free() override;
};

NS_END
