# ShinsuSenju 5연사 Arm 프로젝타일 가이드

## 목적

`Skill_ShinsuSenju`에서 본체를 한 번 소환한 뒤, 플레이어 전방 일정 범위 안으로 `ShinsuSenju Arm` 프로젝타일을 `타다다다닥` 5발 연속 발사하는 구조를 기준으로 정리한 가이드다.

핵심은 아래 2개다.

1. `Skill_ShinsuSenju`는 "연출 + 5연사 스케줄러" 역할만 맡긴다.
2. 실제로 날아가서 맞는 오브젝트는 새 `Skill_ShinsuSenju_Arm` 프로젝타일 클래스로 분리한다.

---

## 현재 구조 기준 정리

현재 레포 기준으로 이미 준비된 부분은 아래와 같다.

- `Client/Private/Skill_ShinsuSenju.cpp`
  - 본체 스폰 클래스는 이미 있다.
  - 하지만 `Spawn_ShinsuSenju()` / `Spawn_Arm()`는 아직 비어 있다.
- `Client/Bin/Resources/Data/json/DT_GameObject.json`
  - `OBJECT_TYPE_SHINSUSENJU`
  - `OBJECT_TYPE_SHINSUSENJU_ARM`
  - 둘 다 이미 등록돼 있다.
- `Client/Bin/Resources/Data/json/DT_Model.json`
  - `ShinsuSenju`
  - `ShinsuSenju_Arm`
  - 모델 경로도 이미 연결돼 있다.
- `Client/Bin/Resources/Data/json/AnimNotifies/Custom.animnotify.json`
  - `SK_CHR_NormalModel|Hashirama_Ninjutsu_ShinsuSenju` 클립에서
  - `AN_PlayCinematic` 뒤에 `AN_SpawnSkill`로 `OBJECT_TYPE_SHINSUSENJU`를 한 번 스폰한다.

즉, 지금 필요한 건 `애니메이션에서 본체 1회 스폰 -> 본체가 내부 타이머로 Arm 5연사` 흐름을 완성하는 것이다.

---

## 권장 구조

### 1. 역할 분리

`Skill_ShinsuSenju`

- 본체 비주얼 렌더
- 연출 위치 고정
- Arm 발사 타이머 관리
- 발사 목표 범위 계산

`Skill_ShinsuSenju_Arm`

- 실제 이동하는 프로젝타일
- 충돌 판정
- 피격 처리
- 히트 이펙트 / 소멸 처리

### 2. 왜 이 구조가 맞는가

- `AN_SpawnSkill`을 5번 찍는 방식은 애니메이션 수정 비용이 커진다.
- 시네마틱 길이나 재생속도가 바뀌면 타이밍이 같이 깨지기 쉽다.
- 반면 `Skill_ShinsuSenju` 내부 타이머로 5발을 쏘면, 발사 간격과 범위 튜닝을 C++에서 바로 조정할 수 있다.

---

## 권장 발사 규칙

### 1. 발사 타이밍

- 본체 스폰 후 `0.20f ~ 0.30f` 정도 딜레이
- 이후 `0.10f ~ 0.14f` 간격으로 5발 발사

추천값:

- 첫 발사 지연: `0.25f`
- 발사 간격: `0.12f`
- 발사 수: `5`

### 2. 발사 범위

플레이어 전방 기준으로 "중심점 + 산포" 방식이 제일 다루기 쉽다.

- 전방 중심 거리: `16.f ~ 20.f`
- 좌우 산포: `-6.f ~ +6.f`
- 높이 산포: `-1.5f ~ +3.f`
- 깊이 산포: `-2.f ~ +2.f`

추천은 완전 랜덤보다 `고정 패턴 + 약한 랜덤`이다.

예시 패턴:

- 1발: 왼쪽 넓게
- 2발: 왼쪽 약간
- 3발: 중앙
- 4발: 오른쪽 약간
- 5발: 오른쪽 넓게

이렇게 해야 "의도된 연사" 느낌이 난다.

### 3. 조준 규칙

1차 구현은 아래처럼 단순하게 가는 걸 추천한다.

- 락온 여부와 무관하게 `플레이어 전방 기준 범위`에 탄착점을 만든다.
- 각 Arm은 "탄착점 방향"으로 날아간다.

2차 확장으로는 아래를 붙일 수 있다.

- 락온 타겟이 있으면 중심점을 타겟 근처로 보정
- 단, 타겟이 너무 멀면 다시 전방 고정 범위로 복귀

---

## 구현 포인트

### [추가] `Client/Public/Skill_ShinsuSenju.h`

아래처럼 "버스트 설정값"과 "발사용 helper"를 넣는 방향을 추천한다.

```cpp
#pragma once

#include "SkillObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class Skill_ShinsuSenju : public SkillObject
{
    GENERATED_BODY(Skill_ShinsuSenju)

public:
    explicit Skill_ShinsuSenju(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_ShinsuSenju(const Skill_ShinsuSenju& rhs);
    virtual ~Skill_ShinsuSenju() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

private:
    HRESULT Ready_Components();
    HRESULT Bind_ShaderResources();

    // Arm 연사가 시작되기 전에 목표 중심점을 한 번 계산한다.
    void    Resolve_BurstCenter();

    // 현재 발사 인덱스에 맞는 탄착 목표점을 만든다.
    Vec3    Build_ArmTargetPoint(int32 burstIndex) const;

    // 다음 Arm 한 발을 실제로 생성하고 카운트를 증가시킨다.
    void    Fire_NextArm();

    // Arm 프로젝타일을 월드에 스폰하고 목표점을 향해 발사한다.
    void    Spawn_Arm(const Vec3& targetPoint);

private:
    Shared<Shader> _shader;
    Shared<Model>  _model;

private:
    // 본체 소환 후 첫 Arm 발사 전 연출 대기 시간이다.
    float   _initialFireDelay = 0.25f;

    // Arm 5연사 간격이다.
    float   _burstInterval = 0.12f;

    // 발사할 총 Arm 개수다.
    int32   _maxBurstCount = 5;

    // 플레이어 전방으로 연사 중심점을 잡는 거리다.
    float   _forwardRange = 18.f;

    // 좌우 산포 범위다.
    float   _sideSpread = 6.f;

    // 상하 산포 범위다.
    float   _heightSpread = 3.f;

    // Arm 프로젝타일 속도다.
    float   _armSpeed = 42.f;

    // Arm 프로젝타일 최대 비행 거리다.
    float   _armMaxDistance = 24.f;

    // 5연사의 기준이 되는 월드 중심점이다.
    Vec3    _burstCenter = Vec3::Zero;

    // 첫 발사 대기 누적 시간이다.
    float   _delayElapsed = 0.f;

    // 직전 발사 후 경과 시간이다.
    float   _burstElapsed = 0.f;

    // 지금까지 발사한 Arm 수다.
    int32   _burstCount = 0;

    // 5연사가 끝났는지 여부다.
    bool    _isBurstFinished = false;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### [추가] `Client/Private/Skill_ShinsuSenju.cpp`

핵심은 `Update()`에서 타이머를 돌리며 `Fire_NextArm()`를 호출하는 것이다.

```cpp
void Skill_ShinsuSenju::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy() || _isBurstFinished)
        return;

    _delayElapsed += timeDelta;
    if (_delayElapsed < _initialFireDelay)
        return;

    _burstElapsed += timeDelta;

    while (_burstCount < _maxBurstCount && _burstElapsed >= _burstInterval)
    {
        _burstElapsed -= _burstInterval;
        Fire_NextArm();
    }

    if (_burstCount >= _maxBurstCount)
    {
        _isBurstFinished = true;
        Set_Destroy(true);
    }
}

void Skill_ShinsuSenju::Resolve_BurstCenter()
{
    auto owner = Get_Owner();
    auto ownerTransform = owner ? owner->Get_Transform() : nullptr;
    if (!ownerTransform)
        return;

    Vec3 forward = ownerTransform->Get_WorldForward();
    forward.y = 0.f;
    forward = Utils::Safe_Normalize(forward, Vec3::Forward);

    _burstCenter = ownerTransform->Get_WorldPosition()
        + forward * _forwardRange
        + Vec3(0.f, 2.0f, 0.f);
}

Vec3 Skill_ShinsuSenju::Build_ArmTargetPoint(int32 burstIndex) const
{
    static const float kSideOffsets[5] = { -6.f, -3.f, 0.f, 3.f, 6.f };
    static const float kHeightOffsets[5] = { 1.5f, 0.5f, 2.5f, 0.0f, 1.0f };
    static const float kDepthOffsets[5] = { -1.0f, 0.5f, 1.0f, -0.5f, 0.0f };

    auto owner = Get_Owner();
    auto ownerTransform = owner ? owner->Get_Transform() : nullptr;
    if (!ownerTransform)
        return _burstCenter;

    const Vec3 right = ownerTransform->Get_WorldRight();
    const Vec3 forward = ownerTransform->Get_WorldForward();

    return _burstCenter
        + right * kSideOffsets[burstIndex]
        + Vec3(0.f, kHeightOffsets[burstIndex], 0.f)
        + forward * kDepthOffsets[burstIndex];
}

void Skill_ShinsuSenju::Fire_NextArm()
{
    const Vec3 targetPoint = Build_ArmTargetPoint(_burstCount);
    Spawn_Arm(targetPoint);
    ++_burstCount;
}

void Skill_ShinsuSenju::Spawn_Arm(const Vec3& targetPoint)
{
    auto owner = Get_Owner();
    auto ownerTransform = owner ? owner->Get_Transform() : nullptr;
    if (!owner || !ownerTransform || !_transformCom)
        return;

    const Vec3 armSpawnPos =
        _transformCom->Get_WorldPosition()
        + _transformCom->Get_WorldUp() * 8.f
        + _transformCom->Get_WorldForward() * 6.f;

    Vec3 direction = targetPoint - armSpawnPos;
    direction = Utils::Safe_Normalize(direction, ownerTransform->Get_WorldForward());

    SkillObject_Projectile::FProjectileSkillDesc desc{};
    desc.ownerObject = owner;
    desc.spawnPosition = armSpawnPos;
    desc.direction = direction;
    desc.speed = _armSpeed;
    desc.maxDistance = _armMaxDistance;
    desc.lifetime = 2.0f;
    desc.colliderRadius = 1.25f;
    desc.collisionPreset = Collision_Preset::Player_Attack;
    desc.scale = Vec3(1.f, 1.f, 1.f);

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_SHINSUSENJU_ARM,
        GAME->Current_Level(),
        TEXT("Layer_Skill"),
        &desc);

    auto arm = dynamic_pointer_cast<SkillObject_Projectile>(spawned);
    if (!arm)
        return;

    arm->Set_Owner(owner);
    arm->Launch(direction);
}
```

포인트는 아래다.

- `AN_SpawnSkill`은 그대로 둔다.
- `Skill_ShinsuSenju`가 스스로 `OBJECT_TYPE_SHINSUSENJU_ARM`를 5번 생성한다.
- `Set_Destroy(true)`는 5발을 다 쏜 뒤에만 호출한다.

### [추가] `Client/Public/Skill_ShinsuSenju_Arm.h`

Arm은 전용 클래스로 분리하는 걸 추천한다.

```cpp
#pragma once

#include "SkillObject_Projectile.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class Skill_ShinsuSenju_Arm : public SkillObject_Projectile
{
    GENERATED_BODY(Skill_ShinsuSenju_Arm)

public:
    explicit Skill_ShinsuSenju_Arm(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_ShinsuSenju_Arm(const Skill_ShinsuSenju_Arm& rhs);
    virtual ~Skill_ShinsuSenju_Arm() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    HRESULT Render() override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    // Arm 전용 모델과 셰이더를 준비한다.
    HRESULT Ready_Components();

    // 렌더링에 필요한 월드/뷰/투영 행렬을 바인딩한다.
    HRESULT Bind_ShaderResources();

private:
    Shared<Shader> _shader;
    Shared<Model>  _model;

    // Arm 피격 시 띄워줄 타격 이펙트 이름이다.
    string _hitEffectName = "Wood_Hit";

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### [추가] `Client/Private/Skill_ShinsuSenju_Arm.cpp`

이 클래스는 사실상 `Skill_FireBall`과 `SkillObject_Projectile`의 중간쯤 되는 구현이면 충분하다.

```cpp
#include "pch.h"
#include "Skill_ShinsuSenju_Arm.h"

#include "GameObject_Factory.h"
#include "Shader.h"
#include "Model.h"
#include "Collider.h"
#include "Character.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_ShinsuSenju_Arm, Protocol::OBJECT_TYPE_SHINSUSENJU_ARM, "SkillSpawn");

HRESULT Skill_ShinsuSenju_Arm::Initialize_Prototype()
{
    _speed = 42.f;
    _maxDistance = 24.f;
    _lifetime = 2.f;
    _colliderRadius = 1.25f;
    _maxHitCount = 1;
    _collisionPreset = Collision_Preset::Player_Attack;

    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_ShinsuSenju_Arm::Initialize(void* arg)
{
    CHECK_FAILED(Ready_Components(), E_FAIL);
    CHECK_FAILED(SkillObject_Projectile::Initialize(arg), E_FAIL);

    return S_OK;
}

void Skill_ShinsuSenju_Arm::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    if (!other || Is_Destroy())
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner || otherOwner == Get_Owner())
        return;

    auto character = dynamic_cast<Character*>(otherOwner.get());
    if (!character)
        return;

    if (!Apply_Skill_Hit(character, 18.f, 2.5f, 0.5f))
        return;

    Spawn_Effect_Once(_hitEffectName, _transformCom->Get_WorldPosition(), Vec3(1.2f));
    Set_Destroy(true);
}
```

여기서는 Arm 모델을 직접 렌더해도 되고, 1차는 이펙트만으로 처리해도 된다.

하지만 지금 `ShinsuSenju_Arm.meshbin`이 이미 있으니, 가능하면 전용 렌더까지 붙이는 게 좋다.

---

## 수정 대상 파일

실제 구현 시 수정될 가능성이 높은 파일은 아래다.

- `Client/Public/Skill_ShinsuSenju.h`
- `Client/Private/Skill_ShinsuSenju.cpp`
- `Client/Public/Skill_ShinsuSenju_Arm.h`
- `Client/Private/Skill_ShinsuSenju_Arm.cpp`
- `Client/Default/Client.vcxproj`
- `Client/Default/Client.vcxproj.filters`

상황에 따라 조정될 수 있는 파일:

- `Client/Bin/Resources/Data/json/AnimNotifies/Custom.animnotify.json`
  - 본체 스폰 타이밍을 조금 앞당기거나 뒤로 미세 조정할 때만 수정
- `Client/Bin/Resources/Data/json/Cinematics/Skill_ShinsuSenju.json`
  - 첫 발사 타이밍과 카메라를 맞추고 싶을 때만 수정

---

## 구현 순서 추천

1. `Skill_ShinsuSenju_Arm` 클래스를 먼저 만든다.
2. `Client.vcxproj` / `filters`에 새 파일을 등록한다.
3. `Skill_ShinsuSenju`에서 `Resolve_BurstCenter`, `Fire_NextArm`, `Spawn_Arm`를 완성한다.
4. 실제 게임에서 5발 간격, 전방 범위, 높이 산포를 튜닝한다.
5. 마지막에 필요하면 `AnimNotify` 시점과 `Cinematic` 프레임만 맞춘다.

---

## 튜닝 체크포인트

아래 5개만 먼저 보면 감이 빨리 온다.

- Arm이 너무 넓게 퍼지면 "난사"처럼 보이고, 너무 좁으면 "중복타"처럼 보인다.
- Arm 속도가 너무 빠르면 투사체 감이 없고, 너무 느리면 박력이 죽는다.
- 본체 스폰 직후 첫 발이 너무 빠르면 시네마틱이 묻힌다.
- `Set_Destroy(true)`를 너무 빨리 하면 본체가 Arm 발사 전에 사라질 수 있다.
- 충돌 반경이 너무 크면 5발이 사실상 한 번에 다 맞는 느낌이 된다.

---

## 한 줄 결론

이번 진수천수는 `AN_SpawnSkill`로 본체 `Skill_ShinsuSenju`를 한 번만 만들고, 그 안에서 새 `Skill_ShinsuSenju_Arm` 프로젝타일을 전방 범위로 5연사하는 구조로 가는 게 가장 덜 꼬이고 튜닝도 쉽다.

---

## [변경] 최신 요구사항 재정의

방금 기준으로 보면, 내가 앞에서 정리한 "전방으로 날아가는 5연사 투사체" 해석은 반만 맞았다.

지금 네가 원하는 건 이쪽에 더 가깝다.

- 플레이어 앞쪽 월드 위치에 `Sphere Radius 10` 정도의 타격 구역을 잡는다.
- Arm은 그 구역 안의 임의 지점을 향해 가는 게 아니라, `위에서 아래로`, 즉 `땅을 향해 팡` 하고 내려꽂힌다.
- 중요한 건 "전방 타겟을 향해 수평 발사"가 아니라, "전방 범위 내 지면 낙하 공격"이다.

즉 구조를 다시 짧게 말하면 아래다.

- `플레이어 앞쪽에 구형 타격 범위 생성`
- `그 범위 안에서 5개의 지면 타격 지점 선정`
- `각 지점 위 상공에서 Arm 생성`
- `Arm이 아래 방향으로 낙하`
- `바닥이나 적에 닿으면 타격 + 이펙트`

---

## [변경] 이번 요구사항에 맞는 권장 구조

이 버전에서는 `수평 직진형 Projectile`보다 `낙하형 Ground Strike Projectile`로 보는 게 맞다.

`Skill_ShinsuSenju`

- 전방 기준 중심점 계산
- `Radius 10` 구형 범위 안에서 5개 타격 지점 선택
- 각 타격 지점의 지면 높이 보정
- 각 지점 상공에서 Arm 낙하 생성

`Skill_ShinsuSenju_Arm`

- 생성 시 이미 목표 지점이 정해진다
- 방향은 거의 항상 `Vec3(0.f, -1.f, 0.f)` 또는 목표 지점 기준 하강
- 바닥 또는 적 충돌 시 타격
- 충돌 즉시 소멸

---

## [변경] 범위 해석

네가 말한 `Sphere Radius 10`은 "플레이어 앞의 한 점을 중심으로 한 구형 샘플링 범위"로 해석하는 게 가장 자연스럽다.

추천 중심점:

- 플레이어 위치 + 전방 `12.f ~ 16.f`
- 높이는 플레이어 기준 `+8.f ~ +12.f`로 올리지 말고, 지면 샘플링용 중심은 우선 `플레이어 발밑 높이 + 1.f` 정도로 둔다.

추천 값:

- 중심 거리: `14.f`
- 구 반지름: `10.f`
- 낙하 시작 높이: `+18.f ~ +25.f`
- Arm 개수: `5`
- 발사 간격: `0.08f ~ 0.14f`

중요한 건 타격 지점을 뽑을 때는 Sphere 안 임의의 3D 점을 그대로 쓰지 말고, `XZ 평면 기준 범위 샘플 -> 그 위치의 지면 높이 찾기` 순서로 가는 거다.

그래야 Arm이 허공이나 벽 중간에 박히지 않고 "땅을 향해 꽂히는" 느낌이 난다.

---

## [추가] 실제 로직 흐름

이번 요구사항에 맞는 실제 흐름은 아래 순서가 맞다.

1. `Skill_ShinsuSenju`가 스폰된다.
2. 플레이어 전방 기준 중심점 `_impactCenter`를 계산한다.
3. `_impactCenter` 기준 반경 `10.f` 안에서 타격 후보점 5개를 만든다.
4. 각 후보점에 대해 위에서 아래로 지면 탐색을 해서 실제 `groundPoint`를 얻는다.
5. `groundPoint + Vec3(0.f, fallHeight, 0.f)` 위치에서 Arm을 생성한다.
6. Arm은 아래로 떨어진다.
7. Arm이 땅 또는 적에 닿으면 이펙트와 데미지를 처리한다.

---

## [변경] `Skill_ShinsuSenju` 쪽에서 바뀌는 핵심

이제 `Build_ArmTargetPoint()`는 "맞출 지점"을 만드는 함수가 되어야 하고, `Spawn_Arm()`은 "그 지점 위에서 아래로 떨구는 함수"가 되어야 한다.

즉 이전 가이드의 핵심 차이는 이거다.

- 이전: `spawnPos -> targetPoint` 방향으로 날림
- 변경: `targetPoint 위 상공 spawnPos -> 아래로 낙하`

예시는 이런 식이 맞다.

```cpp
// 전방 범위의 중심점이다.
void Skill_ShinsuSenju::Resolve_ImpactCenter()
{
    auto owner = Get_Owner();
    auto ownerTransform = owner ? owner->Get_Transform() : nullptr;
    if (!ownerTransform)
        return;

    Vec3 forward = ownerTransform->Get_WorldForward();
    forward.y = 0.f;
    forward = Utils::Safe_Normalize(forward, Vec3::Forward);

    _impactCenter = ownerTransform->Get_WorldPosition() + forward * 14.f;
}

// Radius 10 안에서 한 점을 뽑고, 실제 지면 위치를 찾는다.
Vec3 Skill_ShinsuSenju::Build_GroundImpactPoint(int32 burstIndex) const
{
    static const Vec2 kPattern[5] =
    {
        Vec2(-6.f, -2.f),
        Vec2(-3.f,  3.f),
        Vec2( 0.f,  0.f),
        Vec2( 3.f, -3.f),
        Vec2( 6.f,  2.f),
    };

    Vec3 samplePos = _impactCenter + Vec3(kPattern[burstIndex].x, 0.f, kPattern[burstIndex].y);

    Vec3 groundPoint = samplePos;

    // TODO:
    // 위에서 아래로 라인트레이스 또는 지면 높이 조회를 해서
    // 실제 groundPoint.y를 보정한다.

    return groundPoint;
}

void Skill_ShinsuSenju::Spawn_Arm(const Vec3& groundPoint)
{
    auto owner = Get_Owner();
    if (!owner)
        return;

    const float fallHeight = 22.f;
    const Vec3 spawnPos = groundPoint + Vec3(0.f, fallHeight, 0.f);
    const Vec3 fallDir = Vec3(0.f, -1.f, 0.f);

    SkillObject_Projectile::FProjectileSkillDesc desc{};
    desc.ownerObject = owner;
    desc.spawnPosition = spawnPos;
    desc.direction = fallDir;
    desc.speed = 55.f;
    desc.maxDistance = fallHeight + 3.f;
    desc.lifetime = 1.2f;
    desc.colliderRadius = 1.2f;
    desc.collisionPreset = Collision_Preset::Player_Attack;

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_SHINSUSENJU_ARM,
        GAME->Current_Level(),
        TEXT("Layer_Skill"),
        &desc);

    auto arm = dynamic_pointer_cast<SkillObject_Projectile>(spawned);
    if (!arm)
        return;

    arm->Set_Owner(owner);
    arm->Launch(fallDir);
}
```

---

## [변경] `Skill_ShinsuSenju_Arm`에서 중요한 점

이 Arm은 일반적인 정면 발사형보다 "낙하형 히트 판정"에 더 가깝다.

그래서 체크포인트가 달라진다.

- 생성 직후부터 아래 방향으로 이동
- 바닥과 닿아도 소멸 가능해야 함
- 적과 먼저 닿으면 그 자리에서 바로 타격 처리
- 바닥 충돌 시 작은 폭발형 범위 데미지를 줄지, 단일 히트로 끝낼지 먼저 결정

개인적으로는 1차 구현은 아래가 안전하다.

- `Arm` 본체 충돌 반경은 작게 유지
- 바닥 충돌 시 `Spawn_Effect_Once(...)`
- 필요하면 그 지점에 짧은 수명의 보조 히트 오브젝트를 한 번 더 생성

---

## [변경] 네 요구사항 기준 한 줄 요약

지금 네가 원하는 건 "Arm이 플레이어 앞 범위 안으로 앞으로 날아가는 것"보다는, `플레이어 앞쪽 Radius 10 범위의 지면 여러 지점에 위에서 아래로 꽂히는 진수천수 Arm 낙하 공격`이다.
