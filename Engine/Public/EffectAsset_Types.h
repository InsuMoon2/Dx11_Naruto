#pragma once

#include "Engine_Typedef.h"

NS_BEGIN(Engine)

enum class EEffectBlendMode : uint8
{
    Translucent = 0,                             // 일반 알파 블렌딩 (SrcAlpha * Inv_SrcAlpha) — Pass 0
    Additive,                                    // 가산 혼합: 검은 배경 투명, 빛 합산 — Pass 1
    Opaque,                                      // 불투명: 뒤쪽 완전 가림 — Pass 2
    END
};

enum class EEffectLayerKind : uint8
{
    Point = 0,                                  // VIBuffer_Particle_Point 기반 빌보드 파티클
    Mesh,                                       // 3D 메쉬 기반 이펙트 (나선환 등)
    BillboardRect,
    END
};

struct FEffectLayerBase
{
    string layerName = "Layer";                // 에디터 표시용 이름
    bool enabled = true;                       // 이 레이어의 활성 여부
    EEffectLayerKind kind = EEffectLayerKind::Point; // 레이어 종류

    /* --- 타이밍 --- */
    float startDelay = 0.f;                    // 이펙트 시작 후 이 레이어가 활성화되기까지의 딜레이(초)
    float duration = -1.f;                     // 레이어 지속 시간. -1이면 이펙트 전체 수명을 따른다
    bool loop = false;                         // 이 레이어만 개별 루프할지 여부

    /* --- 로컬 트랜스폼 (오너 기준) --- */
    Vec3 localPosition = Vec3::Zero;           // 오너 기준 위치 오프셋
    Vec3 localRotation = Vec3::Zero;           // 오너 기준 회전 오프셋 (Euler, degree)
    Vec3 localScale = Vec3(1.f, 1.f, 1.f);     // 오너 기준 스케일
    bool useScaleOverTime = false;
    Vec3 endScale = Vec3(1.f, 1.f, 1.f);

    float scaleDuration = 1.f;
};

struct FEffectPointLayerDesc
{
    /* --- 텍스처 --- */
    string textureGuid;                        // Asset_Manager GUID로 파티클 텍스처 참조

    /* --- 인스턴스 --- */
    uint32 numInstances = 1;                   // 파티클 인스턴스 수
    Vec3 center = Vec3::Zero;                  // 스폰 중심 오프셋
    Vec3 range = Vec3::Zero;                   // 랜덤 산포 범위 (x,y,z)
    Vec2 scale = Vec2(1.f, 1.f);               // 파티클 크기 범위 (min, max)
    Vec2 speed = Vec2(0.f, 0.f);               // 이동 속도 범위 (min, max)
    Vec2 lifeTime = Vec2(0.2f, 0.2f);          // 수명 범위 (min, max)
    Vec3 pivot = Vec3::Zero;                   // 움직임 기준점

    bool isLoop = false;                       // 수명 만료 시 재스폰 여부

    /* --- 블렌드 모드 --- */
    EEffectBlendMode blendMode = EEffectBlendMode::Additive; // 파티클 블렌드 모드

    Vec4 colorTint = Vec4(1.f, 1.f, 1.f, 1.f); // 포인트 파티클 전체에 곱해질 틴트/알파 값이다.
    float opacity = 1.f;                       // Point shader 최종 알파 강도 보정값이다.

    uint8 moveMode = 2;                        // 0=Drop(낙하), 1=Spread(방사), 2=Static(정지)
};

struct FEffectMeshLayerDesc
{
    string modelGuid;                          // Asset_Manager GUID로 메쉬 참조

    string diffuseTextureGuid;                
    string maskTextureGuid;
    string emissiveTextureGuid;
    string opacityTextureGuid;

    EEffectBlendMode blendMode = EEffectBlendMode::Translucent; 

    /* --- UV 스크롤 --- */
    Vec2 uvScrollSpeed = Vec2(0.f, 0.f);       // 초당 UV 이동 속도 (u, v). 소용돌이 회전의 핵심
    Vec2 uvTiling = Vec2(1.f, 1.f);            // UV 타일링 반복 횟수

    Vec4 colorTint = Vec4(1.f, 1.f, 1.f, 1.f); 
    float opacity = 1.f;                       

    Vec3 rotationAxis = Vec3(0.f, 1.f, 0.f);   
    float rotationSpeed = 0.f;                 

    /* --- 프레넬 (가장자리 투명 효과) --- */
    float fresnelPower = 0.f;                  
    float fresnelMultiplier = 1.f;             

    bool twoSided = false;                     // CullNone 적용 여부
};

struct FEffectBillboardLayerDesc
{
    string baseTextureGuid;                    // 중심 소용돌이 문양 텍스처 GUID
    string ringTextureGuid;                    // 외곽 링/보조 문양 텍스처 GUID

    EEffectBlendMode blendMode = EEffectBlendMode::Additive; // 라센간 코어는 Additive를 기본으로 사용한다.

    Vec4 baseTint = Vec4(0.55f, 0.92f, 1.f, 1.f);            // 중심 문양에 곱해질 하늘색 계열 틴트다.
    Vec4 ringTint = Vec4(0.85f, 1.f, 1.f, 1.f);              // 외곽 링에 곱해질 더 밝은 틴트다.

    float baseOpacity = 1.f;                                 // 중심 문양 알파 강도다.
    float ringOpacity = 0.45f;                               // 외곽 링 알파 강도다.

    bool useRing = true;                                     // 외곽 보조 레이어를 함께 그릴지 여부다.
    bool billboardToCamera = true;                           // 매 프레임 카메라를 보게 만들지 여부다.
};

struct FEffectLayerDesc
{
    FEffectLayerBase base;          
    FEffectPointLayerDesc point;             
    FEffectMeshLayerDesc  mesh;
    FEffectBillboardLayerDesc billboard; // 나선환처럼 2D 문양 billdboard 세팅할 때
};

struct FEffectAssetDesc
{
    string effectName = "NewEffect";           // 이펙트 이름
    bool autoPlay = true;                      // 로드 즉시 재생 여부
    float totalDuration = -1.f;                // 전체 이펙트 지속 시간. -1이면 모든 레이어 종료 시 자동 종료
    vector<FEffectLayerDesc> layers;           // 레이어 목록
};



NS_END
