#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL Timer : public Base
{
public:
	explicit Timer();
	virtual ~Timer();

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
	static shared_ptr<Timer> Create();

public:
	virtual void	Free();

};

NS_END
