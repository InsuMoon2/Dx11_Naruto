#include "pch.h"
#include "ANS_ComboWindow.h"
#include "AnimNotify_Factory.h"
#include "PlayerStateMachine.h"
#include "GameObject.h"
#include "PlayerState_Attack.h"

REGISTER_ANIM_NOTIFY_STATE(ANS_ComboWindow)

string ANS_ComboWindow::Get_TypeName() const
{
    return "ANS_ComboWindow";
}

void ANS_ComboWindow::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    auto* owner = context.owner;
    CHECK_NULL(owner);

    auto psm = owner->Get_Component<PlayerStateMachine>();

    // 공격 상태인지 확인
    if (!psm || psm->Get_CurrentStateID() != EPlayerState::Attack_1)
        return;

    auto attackState
            = dynamic_pointer_cast<PlayerState_Attack>(psm->Get_CurrentState());

    if (attackState)
        attackState->Open_ComboWindow();

    LOG_INFO("ComboWindow On Begin! clip={}, prev_sec={}, cur_sec={}, delta={}",
        context.clipName, context.previousTimeSec, context.currentTimeSec, context.deltaTime);
}

void ANS_ComboWindow::On_Tick(const FAnimNotifyContext& context)
{
    // 할게있나

}

void ANS_ComboWindow::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    auto* owner = context.owner;
    CHECK_NULL(owner);

    auto psm = owner->Get_Component<PlayerStateMachine>();

    auto attackState
        = dynamic_pointer_cast<PlayerState_Attack>(psm->Get_CurrentState());

    if (attackState)
        attackState->Close_ComboWindow();

    LOG_INFO("ComboWindow On End! clip={}, cur_sec={}, delta={}",
        context.clipName, context.currentTimeSec, context.deltaTime);
}
