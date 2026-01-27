#include "pch.h"
#include "Timer_Manager.h"
#include "Timer.h"

Timer_Manager::Timer_Manager()
{

}

Timer_Manager::~Timer_Manager()
{

}

HRESULT Timer_Manager::Add_Timer(const wstring& timerTag)
{
	auto timer = Find_Timer(timerTag);

    if (timer != nullptr)
		return E_FAIL;

	timer = Timer::Create();

	if (timer == nullptr)
		return E_FAIL;

	_timers.emplace(timerTag, timer);

	return S_OK;
}

float Timer_Manager::Compute_TimeDelta(const wstring& timerTag)
{
	auto timer = Find_Timer(timerTag);

	if (timer == nullptr)
		return 0.f;

	return timer->Update_Timer();
}

shared_ptr<Timer> Timer_Manager::Find_Timer(const wstring& timerTag)
{
	auto iter = _timers.find(timerTag);

	if (iter == _timers.end())
		return nullptr;

	return iter->second;
}

unique_ptr<Timer_Manager> Timer_Manager::Create()
{
	return make_unique<Timer_Manager>();
}

void Timer_Manager::Free()
{
	Base::Free();

}
