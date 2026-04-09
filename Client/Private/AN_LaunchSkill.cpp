#include "pch.h"
#include "AN_LaunchSkill.h"
#include "AnimNotify_Factory.h"
#include "SkillObject_Projectile.h"
#include "GameObject.h"
#include "Object_Manager.h"
#include "SkillComponent.h"

#include "MyPlayer.h"
#include "TargetComponent.h"

#include "Debug_Manager.h"

REGISTER_ANIM_NOTIFY(AN_LaunchSkill);
IMPLEMENT_REFLECTION(AN_LaunchSkill);

bool AN_LaunchSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_LaunchSkill";
    info.properties.clear();

    PROPERTY_ENUM_JSON("발사 스킬 타입", "launch_type", _launchObjectType, Protocol::OBJECT_TYPE);
    PROPERTY_BOOL_JSON("타겟을 향해 던질지(Y축 대각선)", "aim_at_target", _aimAtTarget);

    return true;
}

void AN_LaunchSkill::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    CHECK_NULL(skillCom);

    Vec3 launchDir = context.owner->Get_Transform()->Get_WorldForward();
    auto projectileObj = skillCom->Get_PendingSkill(_launchObjectType).lock();

    if (_aimAtTarget)
    {
        auto myPlayer = dynamic_cast<MyPlayer*>(context.owner);
        if (myPlayer)
        {
            auto targetCom = myPlayer->Get_Component<TargetComponent>();
            if (targetCom && targetCom->IsLockOn())
            {
                auto lockedTarget = targetCom->Get_LockedTarget().lock();
                if (lockedTarget)
                {
                    Vec3 targetPos = lockedTarget->Get_Transform()->Get_WorldPosition();
                    targetPos.y += 0.5f;

                    Vec3 startPos = projectileObj->Get_Transform()->Get_WorldPosition();

                    {
                        FDebugTraceLineDesc traceDesc{};
                        traceDesc.start = startPos;
                        traceDesc.end = targetPos;
                        traceDesc.duration = 5.f;
                        traceDesc.depthEnabled = true;
                        traceDesc.drawHitPoint = true;
                        traceDesc.drawHitNormal = true;
                        traceDesc.drawRemainderOnHit = true;

                        GAME->Draw_DebugTraceLine(traceDesc);
                    }

                    launchDir = targetPos - startPos;
                    launchDir.Normalize();
                }
            }

        }
    }

    // 투사체 회전
    if (projectileObj)
    {
        auto projTransform = projectileObj->Get_Transform();
        projTransform->LookAt(projTransform->Get_WorldPosition() + launchDir);
    }

    skillCom->Launch_PendingSkill(_launchObjectType, launchDir);
}
