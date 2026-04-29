#pragma once
#include "SkillObject_Projectile.h"

NS_BEGIN(Client)

class Skill_ChibakuTensei : public SkillObject_Projectile
{
    GENERATED_BODY(Skill_ChibakuTensei)

public:
    explicit Skill_ChibakuTensei(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_ChibakuTensei(const Skill_ChibakuTensei& rhs);
    virtual ~Skill_ChibakuTensei() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;

    // BTTask_ChibakuTensei가 히트 타이밍에 타겟에게 돌 부착 시퀀스를 직접 시작할 때 호출한다.
    void    Begin_AttachSequence(Character* target);

    // ANS/Launch 경로로 만들어진 보이지 않는 트리거가 중복 시퀀스를 만들지 않도록 발사 직후 정리한다.
    void    Launch(const Vec3& direction) override;

private:
    struct FAttachedStone
    {
        Weak<GameObject> effectObject;        
        Vec3 startWorldPosition = Vec3::Zero; 
        Vec3 localAttachOffset = Vec3::Zero;  
        Vec3 localRotation = Vec3::Zero;      
        Vec3 localScale = Vec3::One;          
        float spawnTime = 0.f;                
        float attachDuration = 0.25f;         
    };

private:
    Character* Find_HitCharacter(Shared<Collider> other) const;
    void Spawn_ReadyStones();
    void Update_AttachedStones(float timeDelta);
    // 코어가 드러나는 순간 핵심 구체와 추가 임팩트 이펙트를 함께 띄울 때 호출한다.
    void Spawn_CoreShell();
    Shared<GameObject> Spawn_AttachedEffect(const string& effectAssetName, const Vec3& worldPosition, const Vec3& worldRotation, const Vec3& worldScale);
    bool Try_GetTargetCenter(Vec3& outCenter) const;

    // 지폭천성에 붙잡힌 대상을 공중에 고정하고, 종료 시 중력/낙하를 정리할 때 호출한다.
    void Update_TargetHold(float timeDelta);
    // 구속 중 다다다닥 맞는 연출을 위해 짧은 간격으로 피격 이벤트를 누적 발행할 때 호출한다.
    void Update_BindMultiHit(float timeDelta);
    // 구속이 끝나거나 스킬이 강제 종료될 때 대상의 중력 상태를 원복하고 낙하를 열어 줄 때 호출한다.
    void Release_TargetHold(bool forceDrop);
    // 낙하가 시작된 뒤 대상이 지면에 닿는 순간 연막 버스트와 코어 정리를 한 번만 처리할 때 호출한다.
    void Update_LandingBurst(float timeDelta);
    // 착지 시 코어와 돌을 즉시 정리해 지폭천성 구체가 해체되는 연출을 만들 때 호출한다.
    void Cleanup_AttachedEffects();
    // 착지 순간 대상 주변으로 Test_Smoke를 여러 번 퍼뜨릴 때 호출한다.
    void Spawn_LandingSmokeBurst(const Vec3& center);
    // 진수천수 착지 파편처럼 메쉬 조각을 더 크게, 더 많이 뿌릴 때 호출한다.
    void Spawn_LandingMeshDebrisBurst(const Vec3& center);

    Vec3 Build_AttachOffset(int32 stoneIndex) const;
    Vec3 Build_StartOffset(int32 stoneIndex) const;
    Vec3 Build_StoneRotation(int32 stoneIndex) const;
    Vec3 Build_StoneScale(int32 stoneIndex) const;

private:
    Weak<Character> _attachTarget;           
    vector<FAttachedStone> _stones;          
    Weak<GameObject> _coreShell;             

    bool _attachSequenceStarted = false;     
    bool _coreShellSpawned = false;          
    float _sequenceElapsed = 0.f;            
    int32 _nextStoneIndex = 0;               

    int32 _stoneCount = 28;                  
    float _stoneSpawnInterval = 0.045f;      
    float _stoneAttachDuration = 0.28f;      
    float _coreRevealRatio = 0.82f;          
    float _sequenceLifetime = 3.2f;          
    float _attachRadius = 1.05f;             
    float _startRadius = 3.2f;

private:
    // 중력무시 위에 떠있게
    bool _targetHoldStarted = false;
    bool _targetHoldFinished = false;
    bool _hasSavedTargetGravityState = false; // 구속 시작 전에 대상의 중력 상태를 저장했는지 추적한다.
    bool _targetDropTriggered = false; // 구속 종료 시 낙하 해제를 한 번만 수행하기 위한 플래그다.
    bool _landingBurstTriggered = false; // 지면 충돌 마무리 연출을 한 번만 실행하기 위한 플래그다.

    float _targetHoldDelay = 0.5f;
    float _targetHoldDuration = 2.0f;
    float _targetHoldElapsed = 0.f;
    float _releaseDropSpeed = -18.f; // 2초 구속이 끝난 뒤 아래로 툭 떨어지는 느낌을 주는 초기 하강 속도다.

    Vec3 _targetHoldPosition = Vec3::Zero;
    bool _savedTargetGravityEnabled = true; // 구속 전 대상의 중력 활성 상태를 복원할 때 사용한다.

private:
    bool _bindMultiHitStarted = false; // 첫 구속 후 다단히트 루프가 시작됐는지 추적한다.
    float _bindMultiHitDelay = 0.2f; // 공중에 묶인 직후 약간 텀을 두고 다단히트를 시작하기 위한 지연 시간이다.
    float _bindMultiHitInterval = 0.12f; // 다단히트가 다다다닥 들어가는 간격이다.
    float _bindMultiHitElapsed = 0.f; // 현재 다단히트 간격 누적 시간을 저장한다.
    float _bindMultiHitDamage = 0.35f; // 구속 중 반복 히트 1회당 들어가는 보조 데미지다.
    string _bindHitAnimStateOverride = "Hit_Air"; // 구속 중 반복 타격이 공중 피격 상태를 유지하도록 강제할 애니메이션 상태명이다.
    string _coreImpactEffectName = "WoodHand_Impact"; // 코어가 생성될 때 함께 터뜨릴 추가 임팩트 이펙트 이름이다.
    Vec3 _coreImpactEffectScale = Vec3(4.4f, 4.4f, 4.4f); // 코어 생성 시 사용하는 WoodHand_Impact를 기존 대비 2배 크게 보이도록 적용할 스케일이다.
    string _landingSmokeEffectName = "Test_Smoke"; // 지면 착지 순간 주변으로 펑펑 터뜨릴 연막 이펙트 이름이다.
    Vec3 _landingSmokeEffectScale = Vec3(1.8f, 1.8f, 1.8f); // 착지 연막이 충분히 크게 퍼져 보이도록 적용할 스케일이다.
    float _landingSmokeRadius = 2.8f; // 착지 버스트를 원형으로 배치할 때 사용할 반경이다.
    int32 _landingSmokeBurstCount = 6; // 착지 순간 동시에 퍼뜨릴 Test_Smoke 개수다.
    string _landingDebrisEffectName = "SmallRock"; // 착지 순간 바닥에서 튀어 오를 메쉬 파편 이펙트 이름이다.
    int32 _landingDebrisBurstCount = 18; // 진수천수보다 더 과하게 보이도록 한 번에 뿌릴 파편 개수다.
    float _landingDebrisRadius = 3.6f; // 파편이 중심에서 퍼져 나갈 때 사용할 스폰 반경이다.

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
