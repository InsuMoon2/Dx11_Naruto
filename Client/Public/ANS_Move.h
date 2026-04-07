#pragma once
#include "AnimNotifyState.h"

NS_BEGIN(Client)

class ANS_Move : public AnimNotifyState
{
    GENERATED_BODY(ANS_Move)

public:
    enum class EMoveDirectionSource : uint8
    {
        OwnerForward,
        TargetDirection,
        DashInputDirection
    };

public:
    string Get_TypeName() const override;

    void On_Begin(const FAnimNotifyContext& context) override;
    void On_Tick(const FAnimNotifyContext& context)  override;
    void On_End(const FAnimNotifyContext& context)   override;

public:
    json Serialize_Payload()  const override;
    void Deserialize_Payload(const json& payload) override;

private:
    // 마지막 입력 방향에 맞는 맞는 이동 방향을 계산
    bool Set_MoveDirection(const FAnimNotifyContext& context, Vec3& outDir) const;
    // 타겟팅 기준 방향을 계산
    bool Set_TargetDirection(const FAnimNotifyContext& context, Vec3& outDir) const;
    // 상태머신에 저장된 대쉬 방향을 읽기
    bool Set_DashDirection(const FAnimNotifyContext& context, Vec3& outDir) const;

private:
    float _moveSpeed = 0.f;

    bool  _rotateToTarget = true;    
    float _rotationSpeed = 720.f;

    EMoveDirectionSource _directionSource = EMoveDirectionSource::OwnerForward;

    Vec3  _moveDir = Vec3::Zero;

    // Y축 무시할지?
    bool _ignoreY = true;
};

NS_END
