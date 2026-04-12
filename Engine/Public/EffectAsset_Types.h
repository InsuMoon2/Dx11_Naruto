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

enum class EEffectPointSpawnShape : uint8
{
    Box = 0,                                    // 기존 range 박스 안에서 랜덤하게 뿌리는 기본 스폰 형태다.
    Sphere,                                     // 구 안쪽 볼륨에서 랜덤하게 뿌릴 때 사용한다.
    Cylinder,                                   // 원기둥 볼륨에서 랜덤하게 뿌릴 때 사용한다.
    Ring,                                       // 바닥 링/원판 테두리처럼 평면 원형 띠에 뿌릴 때 사용한다.
    END
};

enum class EEffectMeshShadingMode : uint8
{
    Unlit = 0,                                  // 기존처럼 emissive 기반으로만 보이는 이펙트 메쉬 셰이딩이다.
    Lit,                                        // 노멀/러프니스/스페큘러를 반영한 조명 기반 이펙트 메쉬 셰이딩이다.
    END
};

enum class EEffectBillboardRenderMode : uint8
{
    CoreSphere = 0,                             // 라센간/파이어볼 코어처럼 중심이 찬 구체형 Billboard를 그릴 때 사용한다.
    FlipbookDecal,                              // 화염 데칼이나 표면 꼬물거림처럼 텍스처 원형을 그대로 살려 붙일 때 사용한다.
    Distortion,                                 // 열기/왜곡 마스크처럼 부드럽고 옅은 연무형 Billboard를 그릴 때 사용한다.
    END
};

struct FEffectFlipbookDesc
{
    bool enabled = false;                       // true면 이 텍스처 슬롯을 Flipbook/SubUV 방식으로 재생한다.
    int columns = 1;                            // 시트의 가로 프레임 수다.
    int rows = 1;                               // 시트의 세로 프레임 수다.
    float fps = 16.f;                           // 초당 몇 프레임으로 Flipbook을 넘길지 결정한다.
    int startFrame = 0;                         // 재생 시작 프레임 인덱스다.
    int endFrame = -1;                          // -1이면 시트 마지막 프레임까지 사용하고, 아니면 이 프레임에서 끝난다.
    bool loop = true;                           // 마지막 프레임까지 갔을 때 다시 처음으로 돌릴지 결정한다.
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
    EEffectPointSpawnShape spawnShape = EEffectPointSpawnShape::Box; // 이 Point 레이어가 어떤 형태로 인스턴스를 뿌릴지 정한다.
    float spawnRadius = 1.f;                   // Sphere/Cylinder/Ring 모드에서 사용할 바깥 반경이다.
    float spawnInnerRadius = 0.f;              // Sphere/Cylinder/Ring 모드에서 안쪽을 비워야 할 때 쓸 안쪽 반경이다.
    float spawnHeight = 1.f;                   // Cylinder 모드에서 위아래 높이 범위를 결정한다.
    Vec2 scale = Vec2(1.f, 1.f);               // 파티클 크기 범위 (min, max)
    Vec2 speed = Vec2(0.f, 0.f);               // 이동 속도 범위 (min, max)
    Vec2 lifeTime = Vec2(0.2f, 0.2f);          // 수명 범위 (min, max)
    Vec3 pivot = Vec3::Zero;                   // 움직임 기준점

    bool isLoop = false;                       // 수명 만료 시 재스폰 여부

    /* --- 블렌드 모드 --- */
    EEffectBlendMode blendMode = EEffectBlendMode::Additive; // 파티클 블렌드 모드

    Vec4 colorTint = Vec4(1.f, 1.f, 1.f, 1.f); // 포인트 파티클 전체에 곱해질 틴트/알파 값이다.
    float opacity = 1.f;                       // Point shader 최종 알파 강도 보정값이다.
    bool useColorTintOverTime = false;         // true면 colorTint에서 endColorTint로 레이어 수명 동안 보간한다.
    Vec4 endColorTint = Vec4(1.f, 1.f, 1.f, 1.f); // Point 레이어 수명 끝에서 도달할 틴트 색상이다.
    bool useOpacityOverTime = false;           // true면 opacity에서 endOpacity로 레이어 수명 동안 보간한다.
    float endOpacity = 0.f;                    // Point 레이어 수명 끝에서 도달할 최종 opacity 값이다.
    FEffectFlipbookDesc flipbook;              // Point 텍스처를 Flipbook/SubUV 시트로 재생할 때 사용할 설정이다.
    Vec4 customParams0 = Vec4::Zero;           // 셰이더에서 자유롭게 읽을 수 있는 사용자 정의 파라미터 0번 슬롯이다.
    Vec4 customParams1 = Vec4::Zero;           // 셰이더에서 자유롭게 읽을 수 있는 사용자 정의 파라미터 1번 슬롯이다.

    uint8 moveMode = 2;                        // 0=Drop(낙하), 1=Spread(방사), 2=Static(정지)
};

struct FEffectMeshLayerDesc
{
    string modelGuid;                          // Asset_Manager GUID로 메쉬 참조

    string diffuseTextureGuid;                
    string maskTextureGuid;
    string emissiveTextureGuid;
    string opacityTextureGuid;
    string opacitySubUvTextureGuid;
    string opacityGradationTextureGuid;
    string emissiveGradationTextureGuid;
    string uvDistortionTextureGuid;
    string normalTextureGuid;                  // Lit 모드에서 노멀맵으로 사용할 텍스처 GUID다.
    string roughnessTextureGuid;               // Lit 모드에서 표면 거칠기 값을 샘플링할 텍스처 GUID다.
    string specularTextureGuid;                // Lit 모드에서 스페큘러 강도 마스크로 사용할 텍스처 GUID다.

    EEffectBlendMode blendMode = EEffectBlendMode::Translucent; 
    EEffectMeshShadingMode shadingMode = EEffectMeshShadingMode::Unlit; // 이 메쉬 레이어가 unlit/lit 중 어떤 셰이딩 경로를 쓸지 정한다.

    /* --- UV 스크롤 --- */
    Vec2 uvScrollSpeed = Vec2(0.f, 0.f);       // 초당 UV 이동 속도 (u, v). 소용돌이 회전의 핵심
    Vec2 uvTiling = Vec2(1.f, 1.f);            // UV 타일링 반복 횟수
    Vec2 uvDistortionStrength = Vec2(0.f, 0.f);
    Vec2 uvDistortionSpeed = Vec2(0.f, 0.f);

    Vec4 colorTint = Vec4(1.f, 1.f, 1.f, 1.f); 
    float opacity = 1.f;                       
    float normalStrength = 1.f;                // Lit 모드에서 노멀맵 xy 강도를 얼마나 강하게 반영할지 정한다.
    float roughness = 0.5f;                    // 러프니스 텍스처가 없을 때 사용할 기본 거칠기 값이다.
    float specularStrength = 1.f;              // 최종 스페큘러 강도 보정값이다.
    float specularPower = 32.f;                // 러프니스가 0에 가까울 때 사용할 하이라이트 집중도다.
    float emissiveStrength = 1.f;              // emissive 텍스처 밝기를 추가로 증폭/감쇠할 때 쓰는 값이다.
    bool useColorTintOverTime = false;         // true면 colorTint에서 endColorTint로 레이어 수명 동안 보간한다.
    Vec4 endColorTint = Vec4(1.f, 1.f, 1.f, 1.f); // Mesh 레이어 수명 끝에서 도달할 틴트 색상이다.
    bool useOpacityOverTime = false;           // true면 opacity에서 endOpacity로 레이어 수명 동안 보간한다.
    float endOpacity = 0.f;                    // Mesh 레이어 수명 끝에서 도달할 opacity 값이다.
    bool useEmissiveStrengthOverTime = false;  // true면 emissiveStrength에서 endEmissiveStrength로 레이어 수명 동안 보간한다.
    float endEmissiveStrength = 0.f;           // Mesh 레이어 수명 끝에서 도달할 emissive 강도다.

    Vec3 rotationAxis = Vec3(0.f, 1.f, 0.f);   
    float rotationSpeed = 0.f;                 

    /* --- 프레넬 (가장자리 투명 효과) --- */
    float fresnelPower = 0.f;                  
    float fresnelMultiplier = 1.f;             

    bool twoSided = false;                     // CullNone 적용 여부
    bool useOpacityAsTransparency = false;     // Additive 메쉬라도 opacity를 일반 알파 투명도처럼 처리하고 싶을 때 켜는 옵션이다.
    Vec4 customParams0 = Vec4::Zero;           // 셰이더에서 자유롭게 읽을 수 있는 사용자 정의 파라미터 0번 슬롯이다.
    Vec4 customParams1 = Vec4::Zero;           // 셰이더에서 자유롭게 읽을 수 있는 사용자 정의 파라미터 1번 슬롯이다.
};

struct FEffectBillboardLayerDesc
{
    string baseTextureGuid;                    // 중심 소용돌이 문양 텍스처 GUID
    string baseMaskTextureGuid;                // Flipbook/데칼 Billboard가 색 텍스처와 별도로 쓸 마스크 텍스처 GUID다.
    string baseOpacityTextureGuid;             // 중심 Billboard의 최종 알파를 별도 마스크 텍스처로 제어할 때 사용하는 GUID다.
    string baseOpacityGradationTextureGuid;    // 중심 Billboard opacity 마스크 값을 원본 GMO 계열 그라데이션으로 리매핑할 때 사용하는 GUID다.
    string ringTextureGuid;                    // 외곽 링/보조 문양 텍스처 GUID
    string ringOpacityTextureGuid;             // 외곽 링 Billboard의 최종 알파를 별도 마스크 텍스처로 제어할 때 사용하는 GUID다.
    string ringOpacityGradationTextureGuid;    // 외곽 링 opacity 마스크 값을 원본 GMO 계열 그라데이션으로 리매핑할 때 사용하는 GUID다.

    EEffectBlendMode blendMode = EEffectBlendMode::Additive; // 라센간 코어는 Additive를 기본으로 사용한다.
    EEffectBillboardRenderMode renderMode = EEffectBillboardRenderMode::CoreSphere; // Billboard를 코어/데칼/왜곡 중 어떤 방식으로 해석할지 결정한다.

    Vec4 baseTint = Vec4(0.55f, 0.92f, 1.f, 1.f);            // 중심 문양에 곱해질 하늘색 계열 틴트다.
    Vec4 ringTint = Vec4(0.85f, 1.f, 1.f, 1.f);              // 외곽 링에 곱해질 더 밝은 틴트다.

    float baseOpacity = 1.f;                                 // 중심 문양 알파 강도다.
    float ringOpacity = 0.45f;                               // 외곽 링 알파 강도다.
    float baseEmissiveStrength = 1.f;                        // 중심 Billboard 색을 발광처럼 증폭하는 강도다.
    float ringEmissiveStrength = 1.f;                        // 외곽 링 Billboard 색을 발광처럼 증폭하는 강도다.
    bool useBaseOpacityOverTime = false;                     // true면 baseOpacity에서 endBaseOpacity로 레이어 수명 동안 보간한다.
    float endBaseOpacity = 0.f;                              // Billboard 중심 레이어 수명 끝에서 도달할 opacity 값이다.
    bool useRingOpacityOverTime = false;                     // true면 ringOpacity에서 endRingOpacity로 레이어 수명 동안 보간한다.
    float endRingOpacity = 0.f;                              // Billboard 링 레이어 수명 끝에서 도달할 opacity 값이다.
    FEffectFlipbookDesc baseFlipbook;                        // 중심 Billboard 텍스처를 Flipbook/SubUV 시트로 재생할 때 사용할 설정이다.
    FEffectFlipbookDesc ringFlipbook;                        // 링 Billboard 텍스처를 Flipbook/SubUV 시트로 재생할 때 사용할 설정이다.

    bool useRing = true;                                     // 외곽 보조 레이어를 함께 그릴지 여부다.
    bool billboardToCamera = true;                           // 매 프레임 카메라를 보게 만들지 여부다.
    Vec4 customParams0 = Vec4::Zero;                         // 셰이더에서 자유롭게 읽을 수 있는 사용자 정의 파라미터 0번 슬롯이다.
    Vec4 customParams1 = Vec4::Zero;                         // 셰이더에서 자유롭게 읽을 수 있는 사용자 정의 파라미터 1번 슬롯이다.
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
