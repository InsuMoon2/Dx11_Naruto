#pragma once

#include "CBase.h"

NS_BEGIN(Engine)

class CTimer;

class CTimer_Manager : public CBase
{
public:
	explicit CTimer_Manager();
	virtual ~CTimer_Manager();

public:
	HRESULT					Add_Timer(const wstring& timerTag);
	float					Compute_TimeDelta(const wstring& timerTag);

private:
	shared_ptr<CTimer>		Find_Timer(const wstring& timerTag);

private:
	unordered_map<wstring, shared_ptr<CTimer>>		_timers;

public:
	static unique_ptr<CTimer_Manager>	Create();
	virtual void						Free();
};

NS_END
