#include "pch.h"
#include "CTimer_Manager.h"
#include "CTimer.h"

CTimer_Manager::CTimer_Manager()
{

}

CTimer_Manager::~CTimer_Manager()
{

}

HRESULT CTimer_Manager::Add_Timer(const wstring& timerTag)
{
	auto timer = Find_Timer(timerTag);

    if (timer != nullptr)
		return E_FAIL;

	timer = CTimer::Create();

	if (timer == nullptr)
		return E_FAIL;

	_timers.emplace(timerTag, timer);

	return S_OK;
}

float CTimer_Manager::Compute_TimeDelta(const wstring& timerTag)
{
	auto timer = Find_Timer(timerTag);

	if (timer == nullptr)
		return 0.f;

	return timer->Update_Timer();
}

shared_ptr<CTimer> CTimer_Manager::Find_Timer(const wstring& timerTag)
{
	auto iter = _timers.find(timerTag);

	if (iter == _timers.end())
		return nullptr;

	return iter->second;
}

unique_ptr<CTimer_Manager> CTimer_Manager::Create()
{
	return make_unique<CTimer_Manager>();
}

void CTimer_Manager::Free()
{
	CBase::Free();

}
