# 손의 움직임을 따라가는 스트레칭 메쉬 이팩트 가이드라인 (엔진 표준 반영본)

이 문서는 고정된 3D 메쉬를 활용하여 손의 움직임에 따라 실시간으로 늘어나는(Stretching) 효과를 구현하는 **최종 완성형 코드**를 포함합니다. 모든 코드는 엔진 전용 매크로(`NS_BEGIN`, `GENERATED_BODY`, `CHECK_FAILED`, `LOCK_WP` 등)와 명명 규칙을 엄격히 준수합니다.

---

## 1. 개요 및 사전 준비

- **방식**: 메쉬 스트레칭 (Scaling & LookAt)
- **대상**: 번개, 검기, 사슬 등 한곳에 고정되어 특정 지점(손 등)으로 뻗어나가는 이펙트
- **준비물**: 피벗(Pivot)이 시작점(Base)에 위치한 3D 메쉬 모델

---

## 2. StretchingMeshEffect.h (완성본)

```cpp
#pragma once

#include "GameObject.h"

NS_BEGIN(Client)

/**
 * @brief 손의 움직임을 실시간으로 추적하여 메쉬를 스트레칭하는 이펙트 오브젝트입니다.
 */
class StretchingMeshEffect final : public GameObject
{
    GENERATED_BODY(StretchingMeshEffect) // 엔진 리플렉션 및 이름 정의 매크로

public:
    struct FStretchingMeshDesc : public FGameObjectDesc
    {
        Shared<Transform>   targetTransform;      // 이펙트가 실시간으로 추적하여 늘어날 방향의 대상 트랜스폼
        wstring             strModelTag = L"";    // 리소스 매니저(Asset_Manager)에 등록된 모델 태그
        float               meshOriginalLength = 1.0f; // 모델링 툴에서 확인된 메쉬의 실제 Z축 원본 길이
        Vec3                thickness = Vec3(1.f, 1.f, 1.f); // 메쉬의 X, Y축 기본 두께
    };

public:
    explicit StretchingMeshEffect(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit StretchingMeshEffect(const StretchingMeshEffect& rhs);
    virtual ~StretchingMeshEffect() = default;

public:
    /**
     * @brief 프로토타입 초기화 함수입니다.
     */
    virtual HRESULT Initialize_Prototype() override;

    /**
     * @brief 인스턴스 초기화 함수입니다. 설정 데이터를 기반으로 컴포넌트를 준비합니다.
     */
    virtual HRESULT Initialize(void* arg) override;

    /**
     * @brief 우선순위 업데이트 로직입니다.
     */
    virtual void    Priority_Update(float timeDelta) override;

    /**
     * @brief 매 프레임 타겟을 추적하여 메쉬의 회전과 스케일을 연산합니다.
     */
    virtual void    Update(float timeDelta) override;

    /**
     * @brief 후처리 업데이트 로직이며 렌더 그룹 등록을 수행합니다.
     */
    virtual void    Late_Update(float timeDelta) override;

    /**
     * @brief 실제 화면에 메쉬를 그립니다.
     */
    virtual HRESULT Render() override;

private:
    /**
     * @brief 필요한 컴포넌트(Model, Shader 등)를 추가하고 초기화합니다.
     */
    HRESULT Ready_Components(const wstring& strModelTag);

private:
    Weak<Transform>     _targetTransform;      // 타겟 트랜스폼 (약참조)
    Shared<Model>       _modelCom = nullptr;    // 렌더링할 메쉬 모델 컴포넌트
    Shared<Shader>      _shaderCom = nullptr;   // 이펙트 전용 셰이더 컴포넌트

    float               _meshOriginalLength = 1.0f; // 스케일 계산용 원본 길이
    Vec3                _thickness = Vec3(1.f, 1.f, 1.f); // 이펙트 고유 두께
    Vec3                _originalWorldPos = Vec3::Zero;   // 이펙트 시작 고정 좌표

public:
    static  Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
```

---

## 3. StretchingMeshEffect.cpp (완성본)

```cpp
#include "pch.h"
#include "StretchingMeshEffect.h"
#include "GameObject_Factory.h"
#include "Renderer.h"
#include "Model.h"
#include "Shader.h"

NS_BEGIN(Client)

// 엔진 팩토리 시스템에 이펙트 카테고리로 자동 등록
REGISTER_GAMEOBJECT_CATEGORY(StretchingMeshEffect, Protocol::OBJECT_TYPE_EFFECT_MESH, "Effect");

StretchingMeshEffect::StretchingMeshEffect(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

StretchingMeshEffect::StretchingMeshEffect(const StretchingMeshEffect& rhs)
    : GameObject(rhs)
    , _meshOriginalLength(rhs._meshOriginalLength)
    , _thickness(rhs._thickness)
{
}

HRESULT StretchingMeshEffect::Initialize_Prototype()
{
    return S_OK;
}

HRESULT StretchingMeshEffect::Initialize(void* arg)
{
    CHECK_NULL(arg, E_FAIL); // NULL 체크 매크로 준수

    FStretchingMeshDesc* pDesc = static_cast<FStretchingMeshDesc*>(arg);
    CHECK_FAILED(GameObject::Initialize(pDesc), E_FAIL); // FAILED 체크 매크로 준수

    _targetTransform = pDesc->targetTransform;
    _meshOriginalLength = pDesc->meshOriginalLength;
    _thickness = pDesc->thickness;

    CHECK_FAILED(Ready_Components(pDesc->strModelTag), E_FAIL);
    _originalWorldPos = Get_Transform()->Get_WorldPosition();

    return S_OK;
}

void StretchingMeshEffect::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void StretchingMeshEffect::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    // LOCK_WP 매크로를 사용하여 안전하게 타겟 추적 루틴 수행
    LOCK_WP(_targetTransform, target); 
    
    Vec3 vStart = _originalWorldPos; 
    Vec3 vEnd = target->Get_WorldPosition(); 
    Vec3 vDir = vEnd - vStart;
    float distance = vDir.Length();

    if (distance > 0.001f)
    {
        // 1. 방향 정렬: 대상을 바라보게 처리
        Get_Transform()->LookAt(vEnd);

        // 2. 스케일 조절: Z축(진행축)을 거리 비율에 맞춰 확대
        float scaleZ = distance / _meshOriginalLength;
        Get_Transform()->Set_LocalScale(_thickness.x, _thickness.y, scaleZ);
    }
}

void StretchingMeshEffect::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (nullptr != _pRenderer)
        _pRenderer->Add_RenderGroup(ERenderGroup::Blend, GetSharedPtr());
}

HRESULT StretchingMeshEffect::Render()
{
    CHECK_NULL(_modelCom, E_FAIL);
    CHECK_NULL(_shaderCom, E_FAIL);

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    size_t numMeshes = _modelCom->Get_NumMeshes();
    for (size_t i = 0; i < numMeshes; ++i)
    {
        CHECK_FAILED(_shaderCom->Begin_Pass(1), E_FAIL); // Additive Pass 실행
        CHECK_FAILED(_modelCom->Render(static_cast<uint32>(i)), E_FAIL);
    }

    return S_OK;
}

HRESULT StretchingMeshEffect::Ready_Components(const wstring& strModelTag)
{
    // 모델 컴포넌트 추가를 위한 해시 키 생성 
    uint32 modelKey = static_cast<uint32>(std::hash<string>{}(Utils::ToString(strModelTag)));
    CHECK_FAILED(Add_Component(modelKey, _modelCom), E_FAIL);

    // 셰이더 컴포넌트 추가 (이펙트 전용 표준 ID 사용)
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_EFFECT_MESH, _shaderCom), E_FAIL);

    return S_OK;
}

Shared<GameObject> StretchingMeshEffect::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto pInstance = make_shared<StretchingMeshEffect>(device, context);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create StretchingMeshEffect");
        return nullptr;
    }
    return pInstance;
}

Shared<GameObject> StretchingMeshEffect::Clone(void* arg)
{
    auto pClone = make_shared<StretchingMeshEffect>(*this);
    if (FAILED(pClone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone StretchingMeshEffect");
        return nullptr;
    }
    return pClone;
}

void StretchingMeshEffect::Free()
{
    GameObject::Free();
}

NS_END
```

---

## 4. 활용 방법 (코드 예시)

스킬 로직에서 `GAME->Spawn_GameObject` 호출 시 `FStretchingMeshDesc`에 타겟 핸드 본의 트랜스폼을 넘겨주는 것만으로 이미지와 같은 연출이 가능합니다.

```cpp
/**
 * 활용 예시 (Skill_Chidori.cpp 등에서 사용 시)
 */
void Skill_Chidori::Create_StretchingLightning()
{
    auto pPlayer = dynamic_pointer_cast<Character>(Get_Owner());
    CHECK_NULL(pPlayer);

    const Matrix* pHandSocket = pPlayer->Get_Model()->Get_SocketBoneMatrixPtr("R_Hand_Weapon_cnt_tr");
    CHECK_NULL(pHandSocket);

    StretchingMeshEffect::FStretchingMeshDesc desc{};
    
    // 시작 지점 설정: 현재 플레이어의 손 위치
    desc.transformDesc.localPosition = Vec3::Transform(Vec3::Zero, *pHandSocket); 
    
    // 추적 대상 설정: 플레이어 메인 트랜스폼 또는 별도 타겟
    desc.targetTransform = pPlayer->Get_Transform(); 
    
    desc.strModelTag = L"Model_Lightning_Bolt";
    desc.meshOriginalLength = 5.0f; 
    desc.thickness = Vec3(0.5f, 0.5f, 1.0f);

    GAME->Spawn_GameObject(GAME->Get_CurrentLevelIndex(), L"StretchingMeshEffect", &desc);
}
```

---

## 5. 제작 팁

1. **메쉬 피벗 설정**: 모델링 툴(Blender, Max 등)에서 메쉬의 피벗이 반드시 **(0, 0, 0)의 시작점**에 있어야 합니다. 그래야 스케일 조절 시 한쪽으로만 늘어납니다.
2. **셰이더 파라미터**: `Shader_VtxEffectMesh`의 UV Scroll 속성을 활용하면 메쉬가 늘어남과 동시에 번개 에너지가 흐르는 역동적인 연출이 가능합니다.

## [대체] 6. 최신안: EffectComponent 연동 및 ANS(AnimNotify) 기반 스트레칭 구조

이 최신안은 기존 단일 컴포넌트 모델(`Model`, `Shader` 직접 제어) 코드를 대체하는 가이드입니다.
`.json` 형태의 완성형 이펙트 자산이 제공하는 파티클, 빌보드, 다중 메쉬 레이어 연출을 모두 유지한 채,
**전체 이펙트 오브젝트의 Scale과 LookAt이 타겟을 향해 실시간 스트레칭(Stretching)** 되도록 구조를 개편한 **최종 완성형 코드**입니다.

### [변경] 핵심 로직 설명
1. 기존 `Skill_Chidori.cpp` 등에 섞여있던 위치 추정/생성 로직을 **AnimNotifyState**로 완전 분리하여 에디터 주도형으로 변경했습니다.
2. `StretchingMeshEffect`는 스스로 렌더링하지 않고, 내부적으로 `EffectComponent`를 자식으로 생성하여 JSON의 화려한 연출을 책임지게 합니다.

---

### [대체] 6-1. StretchingMeshEffect.h (최신 하이브리드)
```cpp
#pragma once

#include "GameObject.h"

NS_BEGIN(Client)

class EffectComponent;

/**
 * @brief EffectComponent를 내부적으로 가지고 있어 .json 이펙트 에셋의 모든 레이어(메쉬, 파티클 등)를 
 * 타겟을 향해 방향을 맞추고 늘려주는(스트레칭) 컨테이너 역할의 이펙트입니다.
 */
class StretchingMeshEffect final : public GameObject
{
    GENERATED_BODY(StretchingMeshEffect)

public:
    struct FStretchingMeshDesc : public FGameObjectDesc
    {
        Weak<Transform>     targetTransform;      // 스트레칭될 대상(타겟)의 트랜스폼 참조
        string              effectAssetName = ""; // 재생할 JSON 이펙트 에셋 이름 
        float               meshOriginalLength = 1.0f; // Z축 스트레칭 계산 기준이 되는 이펙트의 기본 길이
        Vec3                thickness = Vec3(1.f, 1.f, 1.f); // 스트레칭 시 굵기 비율
    };

public:
    explicit StretchingMeshEffect(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit StretchingMeshEffect(const StretchingMeshEffect& rhs);
    virtual ~StretchingMeshEffect() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    
    // 내부에 있는 EffectComponent가 자식들을 렌더링하도록 일임하므로, 자체 렌더 함수는 비워둡니다.
    virtual HRESULT Render() override { return S_OK; } 

private:
    /**
     * @brief EffectComponent를 부착하고 제공된 json 에셋 이름으로 애니메이션을 플레이합니다.
     */
    HRESULT Ready_Components(const string& effectAssetName);

private:
    Weak<Transform>         _targetTransform;
    Shared<EffectComponent> _effectCom = nullptr; // 기존 Model, Shader 대신 EffectComponent를 사용 [대체]

    float                   _meshOriginalLength = 1.0f;
    Vec3                    _thickness = Vec3(1.f, 1.f, 1.f);
    Vec3                    _originalWorldPos = Vec3::Zero;   

public:
    static  Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
```

---

### [대체] 6-2. StretchingMeshEffect.cpp (최신 하이브리드)
```cpp
#include "pch.h"
#include "StretchingMeshEffect.h"
#include "EffectComponent.h"
#include "GameObject_Factory.h"

NS_BEGIN(Client)

// 엔진 팩토리 등록 (Protocol::OBJECT_TYPE_STRETCHING_MESH_EFFECT 사용에 주의)
REGISTER_GAMEOBJECT_CATEGORY(StretchingMeshEffect, Protocol::OBJECT_TYPE_STRETCHING_MESH_EFFECT, "Effect");

StretchingMeshEffect::StretchingMeshEffect(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

StretchingMeshEffect::StretchingMeshEffect(const StretchingMeshEffect& rhs)
    : GameObject(rhs)
    , _meshOriginalLength(rhs._meshOriginalLength)
    , _thickness(rhs._thickness)
{
}

HRESULT StretchingMeshEffect::Initialize_Prototype()
{
    return S_OK;
}

HRESULT StretchingMeshEffect::Initialize(void* arg)
{   
    CHECK_NULL(arg, E_FAIL); // NULL 방어 [준수]

    FStretchingMeshDesc* pDesc = static_cast<FStretchingMeshDesc*>(arg);
    CHECK_FAILED(GameObject::Initialize(pDesc), E_FAIL);

    _targetTransform = pDesc->targetTransform;
    _meshOriginalLength = pDesc->meshOriginalLength;
    _thickness = pDesc->thickness;

    // EffectComponent 초기화 및 JSON 에셋 재생 실행
    CHECK_FAILED(Ready_Components(pDesc->effectAssetName), E_FAIL);
    
    // 이펙트 발생 초기 위치 보존
    _originalWorldPos = Get_Transform()->Get_WorldPosition();

    return S_OK;
}

void StretchingMeshEffect::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    LOCK_WP(_targetTransform, target);

    Vec3 vStart = _originalWorldPos; 
    Vec3 vEnd = target->Get_WorldPosition(); 
    Vec3 vDir = vEnd - vStart;
    float distance = vDir.Length();

    if (distance > 0.001f)
    {
        // 부모(현재 객체)를 타겟 방향으로 LookAt 시키고 스케일을 늘리면,
        // 자식 컴포넌트인 EffectComponent의 레이어들(.json)도 부모의 상속을 받아 일괄적으로 변화합니다.
        Get_Transform()->LookAt(vEnd);

        float scaleZ = distance / _meshOriginalLength;
        Get_Transform()->Set_LocalScale(_thickness.x, _thickness.y, scaleZ);
    }
}

void StretchingMeshEffect::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);
}

HRESULT StretchingMeshEffect::Ready_Components(const string& effectAssetName)
{
    // EffectComponent 부착 및 이펙트(.json) 즉시 재생
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_EFFECT, _effectCom), E_FAIL);

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = effectAssetName;
    playDesc.loopOverride = false; // 기본 루핑 설정 (필요시 변경 가능)

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

Shared<GameObject> StretchingMeshEffect::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto pInstance = make_shared<StretchingMeshEffect>(device, context);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create StretchingMeshEffect");
        return nullptr;
    }
    return pInstance;
}

Shared<GameObject> StretchingMeshEffect::Clone(void* arg)
{
    auto pClone = make_shared<StretchingMeshEffect>(*this);
    if (FAILED(pClone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone StretchingMeshEffect");
        return nullptr;
    }
    return pClone;
}

void StretchingMeshEffect::Free()
{
    GameObject::Free();
}

NS_END
```

---

### [추가] 6-3. 에디터 제어용 ANS_StretchingMesh.h
하드코딩 방식을 제거하고 애니메이션 툴에서 직관적으로 노티파이 바를 당겨, 시작 시간과 끝 시간, 메쉬 오프셋까지 지정할 수 있는 기능을 제공합니다.

```cpp
#pragma once

#include "AnimNotifyState.h"

NS_BEGIN(Client)

/**
 * @brief 지정된 애니메이션 구간(State) 동안 이펙트를 손 소켓 등에 붙이고 
 * 타겟의 방향으로 이펙트 전체가 실시간 스트레칭(Stretching) 되도록 제어하는 노티파이입니다.
 */
class ANS_StretchingMesh : public AnimNotifyState
{
    GENERATED_BODY(ANS_StretchingMesh)

public:
    string Get_TypeName() const override { return "ANS_StretchingMesh"; }

    virtual void On_Begin(const FAnimNotifyContext& context) override;
    virtual void On_Tick(const FAnimNotifyContext& context)  override;
    virtual void On_End(const FAnimNotifyContext& context)   override;

private:
    string  _effectAssetName = "Chidori_Bolt"; // Inspector에서 바꿀 수 있는 JSON 이펙트명
    string  _boneName        = "L_Hand_Weapon_cnt_tr"; // 기준 대상이 될 장착 본 이름
    
    float   _originalLength  = 5.0f; // 굵기 정비율 기준 메쉬의 원본 Z크기
    Vec3    _thickness       = Vec3(0.5f, 0.5f, 1.0f); // X, Y축을 축소하거나 부풀릴 세기

    Weak<GameObject> _spawnedEffect;
};

NS_END
```

---

### [추가] 6-4. ANS_StretchingMesh.cpp
에디터용 `PROPERTY_*` 매크로를 연동하고 `Clone_And_Add_GameObject` 팩토리 함수를 사용해 인스턴스화합니다.

```cpp
#include "pch.h"
#include "ANS_StretchingMesh.h"
#include "AnimNotify_Factory.h"
#include "StretchingMeshEffect.h"
#include "Model.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY_STATE(ANS_StretchingMesh)
IMPLEMENT_REFLECTION(ANS_StretchingMesh)

bool ANS_StretchingMesh::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "ANS_StretchingMesh";

    // 에디터의 프로퍼티 윈도우에 노출되어 언제든지 변경이 가능하게 구성합니다.
    PROPERTY_STRING_JSON("이펙트 에셋명", "effect_name", _effectAssetName);
    PROPERTY_STRING_JSON("장착 본 이름",  "bone_name",   _boneName);
    PROPERTY_FLOAT_JSON("원본 길이",      "mesh_length", _originalLength);
    PROPERTY_VEC3_JSON("굵기(두께)",      "thickness",   _thickness, 0.1f);

    return true;
}

void ANS_StretchingMesh::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner || !context.model)
        return;

    // 1. 에디터에서 기입한 소켓(Bone)에서 현재 애니메이션 포즈를 반영한 행렬 획득
    const Matrix* pSocketMatrix = context.model->Get_SocketBoneMatrixPtr(_boneName);
    if (!pSocketMatrix) return;

    // 2. 파라미터 조립
    StretchingMeshEffect::FStretchingMeshDesc desc{};
    // 본 위치 계산 -> 월드로 변환하여 Stretching 시작좌표로 전달
    desc.position           = Vec3::Transform(Vec3::Zero, *pSocketMatrix); 
    desc.targetTransform    = context.owner->Get_Transform(); // 타겟은 기본적으로 자신(주체)으로 설정 (필요시 하드 타겟 포인터로 연동)
    desc.effectAssetName    = _effectAssetName;               // 에디터에서 설정한 json 파일 경로 사용
    desc.meshOriginalLength = _originalLength;
    desc.thickness          = _thickness;

    // 3. 실제 스폰 로직
    auto pSpawned = GAME->Clone_And_Add_GameObject(
        0, 
        Protocol::OBJECT_TYPE_STRETCHING_MESH_EFFECT, 
        TEXT("Layer_Effect"), 
        &desc
    );

    // 4. 추적/삭제 관리를 위해 약참조 등록
    if (pSpawned)
    {
        pSpawned->Set_Owner(context.owner->GetSharedPtr<GameObject>());
        _spawnedEffect = pSpawned;
    }
}

void ANS_StretchingMesh::On_Tick(const FAnimNotifyContext& context)
{
    // 위치 추적 로직 등은 StretchingMeshEffect 클래스의 Update에서 직접 수행합니다.
}

void ANS_StretchingMesh::On_End(const FAnimNotifyContext& context)
{
    // 노티파이 바를 벗어나면 (애니메이션 구간 종료 시) 늘어나던 이펙트 삭제
    auto pEffect = _spawnedEffect.lock();
    if (pEffect)
    {
        pEffect->Set_Destroy(true);
    }
}

NS_END
```
