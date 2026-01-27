#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Timer;

class Timer_Manager : public Base
{
public:
	explicit Timer_Manager();
	virtual ~Timer_Manager();

public:
	HRESULT					Add_Timer(const wstring& timerTag);
	float					Compute_TimeDelta(const wstring& timerTag);

private:
	shared_ptr<Timer>		Find_Timer(const wstring& timerTag);

private:
    using TimerTypes = umap<wstring, shared_ptr<Timer>>;
    TimerTypes _timers;

public:
	static unique_ptr<Timer_Manager>	Create();
	virtual void						Free();
};

NS_END
