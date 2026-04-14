#pragma once

#include "SkillObject.h"

NS_BEGIN(Client)

class Skill_Rasengan_Hit : public SkillObject
{
    GENERATED_BODY(Skill_Rasengan_Hit)

public:
    struct FHitDesc : public FSkillObjectDesc
    {
        Shared<GameObject> damageCauser = nullptr;
    };

public:
    explicit Skill_Rasengan_Hit(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_Rasengan_Hit(const Skill_Rasengan_Hit& rhs);
    virtual ~Skill_Rasengan_Hit() = default;
    
public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnStayOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnEndOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    // 충둘된 놈이 실제로 타격 가능한지 판단
    Character*  Find_HitCharacter(Shared<Collider> other);

    void        Process_MultiHit(Character* hitted, GameObject* targetKey);
    void        Trigger_FinalHit(Character* character, GameObject* targetKey);

private:
    float       _baseScale = 1.0f;
    float       _maxScale = 2.4f;
    float       _scaleGrowSpeed = 2.8f;

    float       _midHitLaunchUp = 4.5f;

    // 터질 때
    float       _finalBlastRadius = 2.5f;
    float       _finalBlastLaunchPower = 140.f;
    float       _finalBlastLaunchUp = 3.5f;

    // 터졌는지?
    bool        _isFinalBlast = false;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};


NS_END
