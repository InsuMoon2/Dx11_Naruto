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
    void Spawn_CoreShell();
    Shared<GameObject> Spawn_AttachedEffect(const string& effectAssetName, const Vec3& worldPosition, const Vec3& worldRotation, const Vec3& worldScale);
    bool Try_GetTargetCenter(Vec3& outCenter) const;

    void Update_TargetHold(float timeDelta);

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

    float _targetHoldDelay = 0.5f;
    float _targetHoldDuration = 1.5f;
    float _targetHoldElapsed = 0.f;

    Vec3 _targetHoldPosition = Vec3::Zero;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
