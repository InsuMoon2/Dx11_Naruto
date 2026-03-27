#include "pch.h"
#include "ANS_CollisionEnable.h"

string ANS_CollisionEnable::Get_TypeName() const
{
    return "ANS_CollisionEnable";
}

void ANS_CollisionEnable::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    // TODO : 무기 충돌체 키기
    LOG_INFO("ANS_CollisionEnable : Begin — 무기 콜리전 활성화");
}

void ANS_CollisionEnable::On_Tick(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;


}

void ANS_CollisionEnable::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview) return;

    // TODO : 무기 충돌체 끄기
    LOG_INFO("ANS_CollisionEnable : End — 무기 콜리전 비활성화");
}
