#include "pch.h"
#include "Customizer_Manager.h"
#include "Player.h"

Customizer_Manager::Customizer_Manager()
{
    // 기본 프리셋 세팅
    Reset_ToDefault();
}

void Customizer_Manager::Reset_ToDefault()
{
}

void Customizer_Manager::Set_Part(ContainerObject::EPartSlot slot, const wstring& modelAssetTag)
{
}

Unique<Customizer_Manager> Customizer_Manager::Create()
{
    return make_unique<Customizer_Manager>();
}

void Customizer_Manager::Free()
{
    Base::Free();
}
