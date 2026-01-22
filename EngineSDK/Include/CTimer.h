#pragma once

#include "CBase.h"

NS_BEGIN(Engine)

class ENGINE_DLL CTimer : public CBase
{
public:
	explicit CTimer();
	virtual ~CTimer();

public:
	HRESULT				Ready_Timer();
	float				Update_Timer();

private:
	LARGE_INTEGER		_frameTime = {};
	LARGE_INTEGER		_fixTime = {};
	LARGE_INTEGER		_lastTime = {};
	LARGE_INTEGER		_cpuTick = {};

	float				_timeDelta = {};

public:
	static shared_ptr<CTimer> Create();

public:
	virtual void	Free();

};

NS_END
