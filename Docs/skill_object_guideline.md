# 스킬 오브젝트 시스템 가이드라인

> 작성일: 2026-03-29
> 목적: 스킬 사용 시 메시를 월드에 소환하는 SkillObject 구현 (즉시 소환형 + 투사체형)

---

## 전체 구조

```
[PlayerState_Skill::Enter / Update]
  └─ SkillObject 스폰 요청
      ├─ [A] 즉시 소환형 (SkillObject_Instant)
      │     └─ 특정 위치에 바로 소환
      │     └─ 일정 시간 뒤 자동 파괴
      │
      └─ [B] 투사체형 (SkillObject_Projectile)
            └─ 소유자 앞에서 출발
            └─ ProjectileComponent로 매 프레임 이동
            └─ 충돌 or 수명 만료 시 파괴
```

---

## 1. 공통 기반: SkillObject (Client)

스킬이 소환하는 게임 오브젝트의 공통 기반 클래스.
`StaticMeshActor`와 비슷한 구조 — Shader + Model을 갖고, 라이프타임 후 자동 파괴.

### 1-1. [NEW] SkillObject.h

```cpp
// Client/Public/SkillObject.h
#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

// 스킬이 소환하는 게임 오브젝트의 공통 기반.
// 메시를 렌더링하고, 지정된 수명(lifetime) 이후 자동 파괴된다.
// 추후 파티클로 교체할 때 이 클래스를 확장하거나 교체한다.
class SkillObject : public GameObject
{
    GENERATED_BODY(SkillObject)

public:
    struct FSkillObjectDesc : public FGameObjectDesc
    {
        wstring modelAssetTag = L"";        // 사용할 모델 에셋 태그
        Vec3    spawnPosition = Vec3::Zero; // 소환 위치 (월드)
        Vec3    spawnRotation = Vec3::Zero; // 소환 회전 (오일러, degree)
        Vec3    scale         = Vec3::One;  // 스케일
        float   lifetime      = 2.f;        // 수명(초). 0이면 수동 파괴 시까지 유지
        int32   ownerSkillId  = 0;          // 소환한 스킬 ID (디버그/판별용)
    };

public:
    explicit SkillObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkillObject(const SkillObject& rhs);
    virtual ~SkillObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;
    HRESULT Bind_ShaderResources() override;

public:
    // 수명이 다 됐거나 외부에서 파괴 요청했는지 확인
    bool    Is_Expired() const { return _isExpired; }

    // 강제로 수명을 만료시킨다 (충돌 시 등)
    void    Force_Expire();

protected:
    // 모델로드 + 셰이더 바인딩
    HRESULT Ready_Components(const wstring& modelAssetTag);
    HRESULT Bind_Lights();

protected:
    Shared<Shader>  _shader;
    Shared<Model>   _model;

    float   _lifetime = 2.f;        // 최대 수명
    float   _elapsedTime = 0.f;     // 경과 시간
    bool    _isExpired = false;     // 수명 만료 여부

    int32   _ownerSkillId = 0;      // 디버그용 스킬 ID

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### 1-1. [변경] SkillObject.h (메시를 버리고 Collider 반영)

```cpp
// Client/Public/SkillObject.h
#pragma once

#include "GameObject.h"
#include "Collider.h"
#include "Bounding_Sphere.h"
#include "Bounding_AABB.h"
#include "Bounding_OBB.h"

NS_BEGIN(Client)

// 스킬 오브젝트 기반. 
// 기존 메시 렌더링 구조를 걷어내고, 충돌체(Collider)만을 부착하여
// 투사체 이동 및 디버그 선형 렌더링을 띄우는 용도입니다.
class SkillObject : public GameObject
{
    GENERATED_BODY(SkillObject)

public:
    struct FSkillObjectDesc : public FGameObjectDesc
    {
        Protocol::ComponentID colliderType = Protocol::COMPONENT_TYPE_COLLIDER_SPHERE; // 소환할 충돌체 모양
        float   colliderRadius = 1.f;                       // 구체형(Sphere)일 때의 반지름
        Vec3    colliderExtents = Vec3(0.5f, 0.5f, 0.5f);   // 박스형(AABB, OBB)일 때의 크기(절반)

        Vec3    spawnPosition = Vec3::Zero; // 소환 위치 (월드)
        Vec3    spawnRotation = Vec3::Zero; // 소환 회전 (오일러)
        Vec3    scale         = Vec3::One;  // 충돌체 스케일
        float   lifetime      = 2.f;        // 수명(초). 0이면 수동 파괴 시까지 유지
        int32   ownerSkillId  = 0;          // 소환한 스킬 ID (디버그/판별용)
    };

public:
    explicit SkillObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkillObject(const SkillObject& rhs);
    virtual ~SkillObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    // HRESULT Render() override; // ❌ 더 이상 메시 연산하지 않으므로 무시

// public:
    // bool    Is_Expired() const { return _isExpired; }      // ❌ GameObject의 Is_Destroy 활용
    // void    Force_Expire();                                // ❌ GameObject의 Set_Destroy(true) 활용

protected:
    // 모델 스폰 대신 충돌체를 생성해서 붙임
    HRESULT Ready_Components(const FSkillObjectDesc& desc);

protected:
    Shared<Collider> _collider; // 핵심: 메시(_model) 대신 _collider를 들고 있음!

    float   _lifetime = 2.f;
    float   _elapsedTime = 0.f;
    // bool    _isExpired = false; // ❌ 삭제: 부모(GameObject)의 플래그를 그대로 사용!!

    int32   _ownerSkillId = 0;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### 1-2. [NEW] SkillObject.cpp

```cpp
// Client/Private/SkillObject.cpp
#include "pch.h"
#include "SkillObject.h"
#include "Model.h"
#include "Shader.h"
#include "Transform.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(SkillObject, Protocol::OBJECT_TYPE_SKILL_OBJECT)

SkillObject::SkillObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

SkillObject::SkillObject(const SkillObject& rhs)
    : GameObject(rhs)
{
}

HRESULT SkillObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT SkillObject::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FSkillObjectDesc*>(arg);
    if (!desc)
        return E_FAIL;

    _lifetime = desc->lifetime;
    _ownerSkillId = desc->ownerSkillId;
    _elapsedTime = 0.f;

    CHECK_FAILED(Ready_Components(desc->modelAssetTag), E_FAIL);

    // Transform 세팅
    if (_transformCom)
    {
        _transformCom->Set_LocalPosition(desc->spawnPosition);
        _transformCom->Set_LocalRotation(desc->spawnRotation.x,
                                          desc->spawnRotation.y,
                                          desc->spawnRotation.z);
        _transformCom->Set_LocalScale(desc->scale);
    }

    return S_OK;
}

void SkillObject::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void SkillObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    // 수명 체크 — lifetime이 0이면 무한 (수동 파괴 전까지 유지)
    if (_lifetime > 0.f)
    {
        _elapsedTime += timeDelta;
        if (_elapsedTime >= _lifetime)
        {
            _isExpired = true;
            Set_Destroy(true);
        }
    }
}

void SkillObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (!_isExpired)
        GAME->Add_RenderGroup(ERenderGroup::NonBlend, this->GetSharedPtr());
}

HRESULT SkillObject::Render()
{
    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    if (!_model)
        return S_OK;

    size_t numMeshes = _model->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; i++)
    {
        _model->Bind_Material(_shader, "g_DiffuseTexture",
                              i, EMaterialTextureSlot::BaseColor, 0);

        CHECK_FAILED(_shader->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_model->Render(i), E_FAIL);
    }

    return S_OK;
}

void SkillObject::Force_Expire()
{
    _isExpired = true;
    Set_Destroy(true);
}

HRESULT SkillObject::Ready_Components(const wstring& modelAssetTag)
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXMESH, _shader), E_FAIL);

    if (!modelAssetTag.empty())
    {
        string tagStr = Utils::ToString(modelAssetTag);
        uint32 modelKey = static_cast<uint32>(std::hash<string>{}(tagStr));
        CHECK_FAILED(Add_Component(modelKey, _model), E_FAIL);
    }

    return S_OK;
}

HRESULT SkillObject::Bind_ShaderResources()
{
    auto worldMatrix = _transformCom->Get_WorldMatrix();
    _shader->Bind_Matrix("g_WorldMatrix", &worldMatrix);
    _shader->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View));
    _shader->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj));

    GAME->Bind_CamPosition(_shader, "g_CamPosition");
    CHECK_FAILED(Bind_Lights(), E_FAIL);

    return S_OK;
}

HRESULT SkillObject::Bind_Lights()
{
    const FLightDesc* lightDesc = GAME->Get_LightDesc(0);
    FLightDesc defaultLight;

    if (!lightDesc)
    {
        defaultLight.direction = Vec4(0.f, -1.f, 1.f, 0.f);
        defaultLight.diffuse   = Vec4(1.f, 1.f, 1.f, 1.f);
        defaultLight.ambient   = Vec4(0.4f, 0.4f, 0.4f, 1.f);
        defaultLight.specular  = Vec4(1.f, 1.f, 1.f, 1.f);
        lightDesc = &defaultLight;
    }

    CHECK_FAILED(_shader->Bind_RawValue("g_LightDir",      &lightDesc->direction, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_LightDiffuse",  &lightDesc->diffuse,   sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_LightAmbient",  &lightDesc->ambient,   sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(_shader->Bind_RawValue("g_LightSpecular", &lightDesc->specular,  sizeof(Vec4)), E_FAIL);

    return S_OK;
}

Shared<GameObject> SkillObject::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SkillObject>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : SkillObject");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> SkillObject::Clone(void* arg)
{
    auto clone = make_shared<SkillObject>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : SkillObject");
        return nullptr;
    }

    return clone;
}

void SkillObject::Free()
{
    GameObject::Free();
}
```

### 1-2. [변경] SkillObject.cpp (메시 렌더링 삭제 및 Collider 연동 반영)

```cpp
// Client/Private/SkillObject.cpp
#include "pch.h"
#include "SkillObject.h"
#include "Transform.h"
#include "GameObject_Factory.h"
#include "GameInstance.h"

REGISTER_GAMEOBJECT(SkillObject, Protocol::OBJECT_TYPE_SKILL_OBJECT)

SkillObject::SkillObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

SkillObject::SkillObject(const SkillObject& rhs)
    : GameObject(rhs)
{
}

HRESULT SkillObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT SkillObject::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FSkillObjectDesc*>(arg);
    if (!desc)
        return E_FAIL;

    _lifetime = desc->lifetime;
    _ownerSkillId = desc->ownerSkillId;
    _elapsedTime = 0.f;

    // Transform 세팅
    if (_transformCom)
    {
        _transformCom->Set_LocalPosition(desc->spawnPosition);
        _transformCom->Set_LocalRotation(desc->spawnRotation.x,
                                          desc->spawnRotation.y,
                                          desc->spawnRotation.z);
        _transformCom->Set_LocalScale(desc->scale);
    }

    // 콜라이더 생성 및 초기화
    CHECK_FAILED(Ready_Components(*desc), E_FAIL);

    return S_OK;
}

void SkillObject::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void SkillObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    // 수명 체크
    if (_lifetime > 0.f)
    {
        _elapsedTime += timeDelta;
        if (_elapsedTime >= _lifetime)
        {
            Set_Destroy(true); // 부모(_isDestroyed)를 true로 전환
        }
    }
}

void SkillObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (!Is_Destroy() && _collider)
    {
        // 매 프레임 위치/회전 상태만 콜라이더에 동기화
        _collider->Update(_transformCom->Get_WorldMatrix());
    }
}

HRESULT SkillObject::Ready_Components(const FSkillObjectDesc& desc)
{
    CHECK_FAILED(Add_Component(desc.colliderType, _collider), E_FAIL);

    if (desc.colliderType == Protocol::COMPONENT_TYPE_COLLIDER_SPHERE)
    {
        Bounding_Sphere::FBoundingSphereDesc sphereDesc;
        sphereDesc.radius = desc.colliderRadius;
        _collider->Initialize(&sphereDesc);
    }
    else if (desc.colliderType == Protocol::COMPONENT_TYPE_COLLIDER_AABB)
    {
        Bounding_AABB::FBoundingAABBDesc boxDesc;
        boxDesc.extents = desc.colliderExtents;
        _collider->Initialize(&boxDesc);
    }
    else if (desc.colliderType == Protocol::COMPONENT_TYPE_COLLIDER_OBB)
    {
        Bounding_OBB::FBoundingOBBDesc boxDesc;
        boxDesc.extents = desc.colliderExtents;
        boxDesc.rotation = desc.spawnRotation; // OBB는 회전 초기값도 지정 가능
        _collider->Initialize(&boxDesc);
    }

    // 🌟 핵심: 콜라이더를 생성할 때 충돌 매니저에 단 1번만 등록합니다!
    // (매니저가 weak_ptr로 들고 있으므로 오브젝트가 죽으면 자동으로 수거됨)
    GAME->Add_Collider(_collider);

    return S_OK;
}

Shared<GameObject> SkillObject::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SkillObject>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : SkillObject");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> SkillObject::Clone(void* arg)
{
    auto clone = make_shared<SkillObject>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : SkillObject");
        return nullptr;
    }

    return clone;
}

void SkillObject::Free()
{
    GameObject::Free();
}
```
```

---

## 2. 투사체 컴포넌트: ProjectileComponent (Engine)

엔진 쪽에 범용 투사체 이동 컴포넌트를 추가합니다.
어떤 GameObject든 붙이면 투사체처럼 이동시킨다.

### 2-1. [NEW] ProjectileComponent.h

```cpp
// Engine/Public/ProjectileComponent.h  (또는 Client/Public/)
#pragma once

#include "Component.h"

NS_BEGIN(Engine)

// 게임 오브젝트를 직선 방향으로 이동시키는 투사체 컴포넌트.
// 소유 오브젝트의 Transform을 매 프레임 방향 * 속도 만큼 이동시킨다.
// 최대 사거리(maxDistance) 또는 최대 수명(maxLifetime) 도달 시
// 오브젝트를 파괴한다.
class ENGINE_DLL ProjectileComponent : public Component
{
    GENERATED_COMPONENT(ProjectileComponent, Protocol::COMPONENT_TYPE_PROJECTILE)

public:
    struct FProjectileDesc
    {
        Vec3    direction = Vec3::Forward;  // 투사체 이동 방향 (정규화 필수)
        float   speed = 20.f;              // 이동 속도 (units/sec)
        float   maxDistance = 50.f;         // 최대 사거리. 0이면 무한
        float   maxLifetime = 5.f;         // 최대 수명(초). 0이면 무한
        bool    useGravity = false;        // 중력 영향 여부 (포물선 궤적용)
        float   gravityScale = 9.8f;       // 중력 가속도 스케일
    };

public:
    explicit ProjectileComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit ProjectileComponent(const ProjectileComponent& rhs);
    virtual ~ProjectileComponent() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

    // 매 프레임 호출하여 위치를 갱신한다.
    // SkillObject_Projectile::Update에서 호출한다.
    void    Update_Projectile(float timeDelta);

public:
    // 지금까지 이동한 총 거리
    float   Get_TraveledDistance() const { return _traveledDistance; }

    // 수명 또는 사거리를 초과했는지
    bool    Is_Expired() const { return _isExpired; }

    // 방향 변경 (유도탄 등에서 사용 가능)
    void    Set_Direction(const Vec3& dir);

    // 속도 변경
    void    Set_Speed(float speed) { _desc.speed = speed; }

    // 디스크립터 세팅 (Initialize 대안용)
    void    Setup(const FProjectileDesc& desc);

private:
    FProjectileDesc _desc;

    float   _traveledDistance = 0.f;    // 누적 이동 거리
    float   _elapsedTime = 0.f;         // 경과 시간
    bool    _isExpired = false;         // 사거리/수명 만료 여부

    Vec3    _velocity = Vec3::Zero;     // 현재 속도 벡터 (중력 포함)

public:
    static Shared<ProjectileComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### 2-1. [변경] ProjectileComponent.h (별도 파괴 변수 제거)

```cpp
// Engine/Public/ProjectileComponent.h  (또는 Client/Public/)
#pragma once

#include "Component.h"

NS_BEGIN(Engine)

// 게임 오브젝트를 직선 방향으로 이동시키는 투사체 컴포넌트.
// 소유 오브젝트의 Transform을 매 프레임 방향 * 속도 만큼 이동시킨다.
// 최대 사거리(maxDistance) 또는 최대 수명(maxLifetime) 도달 시
// "소유자(owner)"를 즉시 파괴한다.
class ENGINE_DLL ProjectileComponent : public Component
{
    GENERATED_COMPONENT(ProjectileComponent, Protocol::COMPONENT_TYPE_PROJECTILE)

public:
    struct FProjectileDesc
    {
        Vec3    direction = Vec3::Forward;  // 투사체 이동 방향 (정규화 필수)
        float   speed = 20.f;              // 이동 속도 (units/sec)
        float   maxDistance = 50.f;         // 최대 사거리. 0이면 무한
        float   maxLifetime = 5.f;         // 최대 수명(초). 0이면 무한
        bool    useGravity = false;        // 중력 영향 여부 (포물선 궤적용)
        float   gravityScale = 9.8f;       // 중력 가속도 스케일
    };

public:
    explicit ProjectileComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit ProjectileComponent(const ProjectileComponent& rhs);
    virtual ~ProjectileComponent() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Update_Projectile(float timeDelta);

public:
    // 지금까지 이동한 총 거리
    float   Get_TraveledDistance() const { return _traveledDistance; }

    // ❌ bool Is_Expired() const 삭제 (Get_Owner()->Is_Destroy() 로 체크 가능하므로 제거)

    void    Set_Direction(const Vec3& dir);
    void    Set_Speed(float speed) { _desc.speed = speed; }
    void    Setup(const FProjectileDesc& desc);

private:
    FProjectileDesc _desc;

    float   _traveledDistance = 0.f;    // 누적 이동 거리
    float   _elapsedTime = 0.f;         // 경과 시간
    // ❌ bool _isExpired = false; 삭제 // 사거리/수명 만료 여부

    Vec3    _velocity = Vec3::Zero;     // 현재 속도 벡터 (중력 포함)

public:
    static Shared<ProjectileComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### 2-2. [NEW] ProjectileComponent.cpp

```cpp
// Engine/Private/ProjectileComponent.cpp  (또는 Client/Private/)
#include "pch.h"
#include "ProjectileComponent.h"
#include "Transform.h"
#include "GameObject.h"

ProjectileComponent::ProjectileComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

ProjectileComponent::ProjectileComponent(const ProjectileComponent& rhs)
    : Component(rhs)
    , _desc(rhs._desc)
{
}

HRESULT ProjectileComponent::Initialize_Prototype()
{
    return Component::Initialize_Prototype();
}

HRESULT ProjectileComponent::Initialize(void* arg)
{
    Component::Initialize(arg);

    if (arg)
    {
        auto* desc = static_cast<FProjectileDesc*>(arg);
        _desc = *desc;
    }

    return S_OK;
}

void ProjectileComponent::BeginPlay()
{
    Component::BeginPlay();

    _traveledDistance = 0.f;
    _elapsedTime = 0.f;
    _isExpired = false;

    // 초기 속도 = 방향 * 스피드
    Vec3 dir = _desc.direction;
    dir.Normalize();
    _velocity = dir * _desc.speed;
}

void ProjectileComponent::Update_Projectile(float timeDelta)
{
    if (_isExpired)
        return;

    auto owner = Get_Owner();
    if (!owner)
        return;

    auto transform = owner->Get_Transform();
    if (!transform)
        return;

    // 중력 적용
    if (_desc.useGravity)
    {
        _velocity.y -= _desc.gravityScale * timeDelta;
    }

    // 이동량 계산
    Vec3 displacement = _velocity * timeDelta;
    float frameDistance = displacement.Length();

    // 위치 갱신
    Vec3 currentPos = transform->Get_LocalPosition();
    transform->Set_LocalPosition(currentPos + displacement);

    // 누적
    _traveledDistance += frameDistance;
    _elapsedTime += timeDelta;

    // 만료 체크
    if (_desc.maxDistance > 0.f && _traveledDistance >= _desc.maxDistance)
    {
        _isExpired = true;
        owner->Set_Destroy(true);
    }

    if (_desc.maxLifetime > 0.f && _elapsedTime >= _desc.maxLifetime)
    {
        _isExpired = true;
        owner->Set_Destroy(true);
    }
}

void ProjectileComponent::Set_Direction(const Vec3& dir)
{
    _desc.direction = dir;
    Vec3 normalized = dir;
    normalized.Normalize();
    _velocity = normalized * _desc.speed;
}

void ProjectileComponent::Setup(const FProjectileDesc& desc)
{
    _desc = desc;

    Vec3 dir = _desc.direction;
    dir.Normalize();
    _velocity = dir * _desc.speed;
}

Shared<ProjectileComponent> ProjectileComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<ProjectileComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : ProjectileComponent");
        return nullptr;
    }

    return instance;
}

Shared<Component> ProjectileComponent::Clone(void* arg)
{
    auto clone = make_shared<ProjectileComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : ProjectileComponent");
        return nullptr;
    }

    return clone;
}

void ProjectileComponent::Free()
{
    Component::Free();
}
```

### 2-2. [변경] ProjectileComponent.cpp (수명/거리 만료 시 부모 파괴 플래그만 세팅)

```cpp
// Engine/Private/ProjectileComponent.cpp  (또는 Client/Private/)
#include "pch.h"
#include "ProjectileComponent.h"
#include "Transform.h"
#include "GameObject.h"

ProjectileComponent::ProjectileComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

ProjectileComponent::ProjectileComponent(const ProjectileComponent& rhs)
    : Component(rhs)
    , _desc(rhs._desc)
{
}

HRESULT ProjectileComponent::Initialize_Prototype()
{
    return Component::Initialize_Prototype();
}

HRESULT ProjectileComponent::Initialize(void* arg)
{
    Component::Initialize(arg);

    if (arg)
    {
        auto* desc = static_cast<FProjectileDesc*>(arg);
        _desc = *desc;
    }

    return S_OK;
}

void ProjectileComponent::BeginPlay()
{
    Component::BeginPlay();

    _traveledDistance = 0.f;
    _elapsedTime = 0.f;
    // _isExpired = false; 삭제

    // 초기 속도 = 방향 * 스피드
    Vec3 dir = _desc.direction;
    dir.Normalize();
    _velocity = dir * _desc.speed;
}

void ProjectileComponent::Update_Projectile(float timeDelta)
{
    auto owner = Get_Owner();
    if (!owner)
        return;

    // 만료된 투사체면 이동하지 않음 (GameObject의 Is_Destroy 활용)
    if (owner->Is_Destroy()) // 매니저에 따라 Is_Dead() 등일 수 있습니다
        return;

    auto transform = owner->Get_Transform();
    if (!transform)
        return;

    // 중력 적용
    if (_desc.useGravity)
    {
        _velocity.y -= _desc.gravityScale * timeDelta;
    }

    // 이동량 계산
    Vec3 displacement = _velocity * timeDelta;
    float frameDistance = displacement.Length();

    // 위치 갱신
    Vec3 currentPos = transform->Get_LocalPosition();
    transform->Set_LocalPosition(currentPos + displacement);

    // 누적
    _traveledDistance += frameDistance;
    _elapsedTime += timeDelta;

    // 만료 체크 (부모만 Set_Destroy 시키면 끝납니다)
    if (_desc.maxDistance > 0.f && _traveledDistance >= _desc.maxDistance)
    {
        owner->Set_Destroy(true);
    }
    else if (_desc.maxLifetime > 0.f && _elapsedTime >= _desc.maxLifetime)
    {
        owner->Set_Destroy(true);
    }
}

void ProjectileComponent::Set_Direction(const Vec3& dir)
{
    _desc.direction = dir;
    Vec3 normalized = dir;
    normalized.Normalize();
    _velocity = normalized * _desc.speed;
}

void ProjectileComponent::Setup(const FProjectileDesc& desc)
{
    _desc = desc;

    Vec3 dir = _desc.direction;
    dir.Normalize();
    _velocity = dir * _desc.speed;
}

Shared<ProjectileComponent> ProjectileComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<ProjectileComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : ProjectileComponent");
        return nullptr;
    }

    return instance;
}

Shared<Component> ProjectileComponent::Clone(void* arg)
{
    auto clone = make_shared<ProjectileComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : ProjectileComponent");
        return nullptr;
    }

    return clone;
}

void ProjectileComponent::Free()
{
    Component::Free();
}
```

---

## 3. 투사체형 스킬 오브젝트: SkillObject_Projectile (Client)

`SkillObject`를 상속하면서 `ProjectileComponent`를 붙여서 이동하는 스킬.

### 3-1. [NEW] SkillObject_Projectile.h

```cpp
// Client/Public/SkillObject_Projectile.h
#pragma once

#include "SkillObject.h"

NS_BEGIN(Engine)
class ProjectileComponent;
NS_END

NS_BEGIN(Client)

// SkillObject에 ProjectileComponent를 붙여서 직선/포물선으로 이동하는 투사체.
// 라센수리검처럼 던지는 스킬에 사용한다.
class SkillObject_Projectile : public SkillObject
{
    GENERATED_BODY(SkillObject_Projectile)

public:
    struct FProjectileSkillDesc : public SkillObject::FSkillObjectDesc
    {
        Vec3    direction = Vec3::Forward;  // 투사체 이동 방향
        float   speed     = 20.f;           // 이동 속도
        float   maxDistance = 50.f;         // 최대 사거리
        bool    useGravity = false;         // 포물선 여부
    };

public:
    explicit SkillObject_Projectile(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkillObject_Projectile(const SkillObject_Projectile& rhs);
    virtual ~SkillObject_Projectile() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

private:
    Shared<ProjectileComponent> _projectile;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### 3-2. [NEW] SkillObject_Projectile.cpp

```cpp
// Client/Private/SkillObject_Projectile.cpp
#include "pch.h"
#include "SkillObject_Projectile.h"
#include "ProjectileComponent.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(SkillObject_Projectile, Protocol::OBJECT_TYPE_SKILL_PROJECTILE)

SkillObject_Projectile::SkillObject_Projectile(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

SkillObject_Projectile::SkillObject_Projectile(const SkillObject_Projectile& rhs)
    : SkillObject(rhs)
{
}

HRESULT SkillObject_Projectile::Initialize_Prototype()
{
    return SkillObject::Initialize_Prototype();
}

HRESULT SkillObject_Projectile::Initialize(void* arg)
{
    // 부모(SkillObject)의 Initialize를 먼저 호출
    // → 모델 로드, Transform 세팅, 수명 설정 등
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FProjectileSkillDesc*>(arg);
    if (!desc)
        return E_FAIL;

    // ProjectileComponent 프로토타입 클론 및 세팅
    ProjectileComponent::FProjectileDesc projDesc;
    projDesc.direction    = desc->direction;
    projDesc.speed        = desc->speed;
    projDesc.maxDistance  = desc->maxDistance;
    projDesc.maxLifetime  = desc->lifetime;       // SkillObject의 lifetime과 공유
    projDesc.useGravity   = desc->useGravity;

    // 🌟 핵심: 엔진 표준 패턴인 Add_Component로 프로토타입 복제!
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_PROJECTILE, _projectile, &projDesc), E_FAIL);

    return S_OK;
}

void SkillObject_Projectile::Update(float timeDelta)
{
    // ProjectileComponent가 위치를 갱신
    if (_projectile)
        _projectile->Update_Projectile(timeDelta);

    // 부모의 Update (수명 체크 등)
    SkillObject::Update(timeDelta);
}

Shared<GameObject> SkillObject_Projectile::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<SkillObject_Projectile>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : SkillObject_Projectile");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> SkillObject_Projectile::Clone(void* arg)
{
    auto clone = make_shared<SkillObject_Projectile>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : SkillObject_Projectile");
        return nullptr;
    }

    return clone;
}

void SkillObject_Projectile::Free()
{
    SkillObject::Free();
}
```

---

## 4. 프로토콜 enum 추가 필요

프로토콜에 새 오브젝트 타입들을 추가해야 합니다.

```protobuf
// Protocol에 추가
OBJECT_TYPE_SKILL_OBJECT = ??;      // 즉시 소환형 스킬 오브젝트
OBJECT_TYPE_SKILL_PROJECTILE = ??;  // 투사체 스킬 오브젝트
```

`COMPONENT_TYPE_PROJECTILE`도 아직 없다면 추가합니다.

---

## 5. 스폰 방법 — 사용 예시

### A. 즉시 소환형 (라센간 히트 이펙트 같은)

```cpp
// PlayerState_Skill이나 AnimNotify에서 호출
void Spawn_InstantSkillObject(GameObject* owner, int32 skillId)
{
    auto transform = owner->Get_Transform();
    Vec3 forwardPos = transform->Get_LocalPosition()
                    + transform->Get_WorldForward() * 2.f;  // 앞 2m

    SkillObject::FSkillObjectDesc desc;
    desc.modelAssetTag = L"Model_Rasengan";      // 임시 메시
    desc.spawnPosition = forwardPos;
    desc.spawnRotation = Vec3::Zero;
    desc.scale         = Vec3(1.f, 1.f, 1.f);
    desc.lifetime      = 1.5f;                   // 1.5초 후 자동 파괴
    desc.ownerSkillId  = skillId;

    // 현재 레벨에 추가
    uint32 levelIndex = GAME->Current_Level();

    GAME->Clone_And_Add_GameObject(
        levelIndex,                              // 프로토타입 레벨
        Protocol::OBJECT_TYPE_SKILL_OBJECT,      // 오브젝트 타입
        levelIndex,                              // 추가할 레벨
        L"Layer_SkillObject",                    // 레이어 태그
        &desc);
}
```

### A. [변경] 즉시 소환형 (모델 대신 구체 충돌체를 이용한 예시)

```cpp
// PlayerState_Skill이나 AnimNotify에서 호출
void Spawn_InstantSkillObject(GameObject* owner, int32 skillId)
{
    auto transform = owner->Get_Transform();
    Vec3 forwardPos = transform->Get_LocalPosition()
                    + transform->Get_WorldForward() * 2.f;  // 앞 2m 위치

    SkillObject::FSkillObjectDesc desc;
    
    // 모델 태그 대신 콜라이더 세부 설정
    desc.colliderType   = Protocol::COMPONENT_TYPE_COLLIDER_SPHERE;
    desc.colliderRadius = 1.0f; // 구체의 반지름
    
    desc.spawnPosition = forwardPos;
    desc.spawnRotation = Vec3::Zero;
    desc.scale         = Vec3(1.f, 1.f, 1.f);
    desc.lifetime      = 1.5f;                   // 1.5초 후 충돌체가 자동 파괴(수거)됨
    desc.ownerSkillId  = skillId;

    uint32 levelIndex = GAME->Current_Level();

    GAME->Clone_And_Add_GameObject(
        levelIndex,                              
        Protocol::OBJECT_TYPE_SKILL_OBJECT,      
        levelIndex,                              
        L"Layer_SkillObject",                    
        &desc);
}
```
```

### B. 투사체형 (라센수리검 던지기 같은)

```cpp
void Spawn_ProjectileSkillObject(GameObject* owner, int32 skillId)
{
    auto transform = owner->Get_Transform();
    Vec3 spawnPos = transform->Get_LocalPosition()
                  + transform->Get_WorldForward() * 1.f   // 앞 1m
                  + Vec3(0.f, 1.f, 0.f);                  // 허리 높이

    Vec3 direction = transform->Get_WorldForward();
    direction.y = 0.f;
    direction.Normalize();

    SkillObject_Projectile::FProjectileSkillDesc desc;
    desc.modelAssetTag = L"Model_RasenShuriken";
    desc.spawnPosition = spawnPos;
    desc.spawnRotation = Vec3::Zero;
    desc.scale         = Vec3(1.f, 1.f, 1.f);
    desc.lifetime      = 3.f;                     // 3초 수명
    desc.ownerSkillId  = skillId;
    desc.direction     = direction;                // 플레이어 정면 방향
    desc.speed         = 25.f;                     // 초속 25
    desc.maxDistance    = 40.f;                     // 최대 40m
    desc.useGravity    = false;                    // 직선 비행

    uint32 levelIndex = GAME->Current_Level();

    GAME->Clone_And_Add_GameObject(
        levelIndex,
        Protocol::OBJECT_TYPE_SKILL_PROJECTILE,
        levelIndex,
        L"Layer_SkillObject",
        &desc);
}
```

### B. [변경] 투사체형 (메시 대신 날아가는 콜라이더 OBB 박스 예시)

```cpp
void Spawn_ProjectileSkillObject(GameObject* owner, int32 skillId)
{
    auto transform = owner->Get_Transform();
    Vec3 spawnPos = transform->Get_LocalPosition()
                  + transform->Get_WorldForward() * 1.f   // 앞 1m
                  + Vec3(0.f, 1.f, 0.f);                  // 허리 높이

    Vec3 direction = transform->Get_WorldForward();
    direction.y = 0.f;
    direction.Normalize();

    SkillObject_Projectile::FProjectileSkillDesc desc;
    
    // 모델 대신 날아가는 OBB 박스를 사용해봅니다.
    desc.colliderType    = Protocol::COMPONENT_TYPE_COLLIDER_OBB;
    desc.colliderExtents = Vec3(0.5f, 0.5f, 1.0f); // 충돌체 폭 / 높이 / 깊이 절반 크기
    
    desc.spawnPosition = spawnPos;
    // OBB의 경우 Rotation값을 주면 상자가 처음부터 회전된 상태로 투사될 수 있습니다.
    desc.spawnRotation = Vec3::Zero;
    desc.scale         = Vec3(1.f, 1.f, 1.f);
    
    desc.lifetime      = 3.f;                     // 3초 수명
    desc.ownerSkillId  = skillId;
    desc.direction     = direction;                // 플레이어 정면 방향
    desc.speed         = 25.f;                     // 초속 25
    desc.maxDistance   = 40.f;                     // 최대 40m
    desc.useGravity    = false;                    // 직선 비행

    uint32 levelIndex = GAME->Current_Level();

    GAME->Clone_And_Add_GameObject(
        levelIndex,
        Protocol::OBJECT_TYPE_SKILL_PROJECTILE,
        levelIndex,
        L"Layer_SkillObject",
        &desc);
}
```

---

## 6. 스폰 타이밍: AnimNotify 활용

스킬 애니메이션의 특정 프레임에서 소환하려면 `AnimNotify`를 사용합니다.

### [NEW] AN_SpawnSkillObject (선택사항, 향후 확장용)

```cpp
// 예시: 노티파이에서 스킬 오브젝트를 소환하는 구조
// AN_SpawnSkillObject::On_Notify(const FAnimNotifyContext& context)
// {
//     auto* owner = context.owner;
//     Spawn_ProjectileSkillObject(owner, skillId);
// }
```

현재 단계에서는 `PlayerState_Skill::Enter` 또는 `Update`에서 직접 호출하는 것이 더 간단합니다. 노티파이 방식은 타이밍 세밀 조정이 필요할 때 추가합니다.

---

## 7. 파일 구조 정리

### 신규 파일

| 위치 | 파일 | 역할 |
|------|------|------|
| Client/Public/ | `SkillObject.h` | 즉시 소환형 스킬 오브젝트 헤더 |
| Client/Private/ | `SkillObject.cpp` | 즉시 소환형 구현 |
| Client/Public/ | `SkillObject_Projectile.h` | 투사체형 스킬 오브젝트 헤더 |
| Client/Private/ | `SkillObject_Projectile.cpp` | 투사체형 구현 |
| Engine/Public/ | `ProjectileComponent.h` | 범용 투사체 이동 컴포넌트 헤더 |
| Engine/Private/ | `ProjectileComponent.cpp` | 투사체 이동 로직 구현 |

### 수정 파일

| 파일 | 변경 |
|------|------|
| Protocol `.proto` | `OBJECT_TYPE_SKILL_OBJECT`, `OBJECT_TYPE_SKILL_PROJECTILE`, `COMPONENT_TYPE_PROJECTILE` 추가 |
| `Loader.cpp` | 스킬 오브젝트 프로토타입 등록 |

---

## 8. 클래스 상속 구조

```
Engine::GameObject
  └─ Client::SkillObject (즉시 소환형)
       ├─ Shader + Model 렌더링
       ├─ lifetime 기반 자동 파괴
       │
       └─ Client::SkillObject_Projectile (투사체형)
             └─ + Engine::ProjectileComponent
                  └─ 방향 * 속도로 매 프레임 이동
                  └─ 사거리/수명 체크

Engine::Component
  └─ Engine::ProjectileComponent (범용 투사체 이동)
```

---

## 9. 추후 확장 방향

| 항목 | 설명 |
|------|------|
| 파티클 교체 | `SkillObject`의 `_model` 대신 `ParticleComponent`를 붙이면 됨 |
| 충돌 | `ColliderComponent`를 추가해서 `On_Hit` → `Force_Expire()` 호출 |
| 유도탄 | `Update`에서 `_projectile->Set_Direction(toTarget)` 매 프레임 갱신 |
| 회전 | `Update`에서 `_transformCom->Set_LocalRotation(...)` 으로 회전 애니 추가 |
| 네트워크 | `Write/Read_ObjectInfo`에서 위치/방향 동기화 |
