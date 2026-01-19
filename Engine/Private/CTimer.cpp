#include "pch.h"
#include "CTimer.h"

CTimer::CTimer() 
	: _timeDelta(0.f)
{
	ZeroMemory(&_fixTime, sizeof(LARGE_INTEGER));
	ZeroMemory(&_lastTime, sizeof(LARGE_INTEGER));
	ZeroMemory(&_frameTime, sizeof(LARGE_INTEGER));
	ZeroMemory(&_cpuTick, sizeof(LARGE_INTEGER));
}

CTimer::~CTimer()
{

}

HRESULT CTimer::Ready_Timer()
{
	QueryPerformanceCounter(&_frameTime);			// 1077
	QueryPerformanceCounter(&_lastTime);			// 1085
	QueryPerformanceCounter(&_fixTime);				// 1090

	QueryPerformanceFrequency(&_cpuTick);			// cpu tick 값을 얻어오는 함수

	return S_OK;
}

float CTimer::Update_Timer()
{
	QueryPerformanceCounter(&_frameTime);			// 1500


	if (_frameTime.QuadPart - _fixTime.QuadPart >= _cpuTick.QuadPart)
	{
		QueryPerformanceFrequency(&_cpuTick);
		_fixTime = _frameTime;
	}


	_timeDelta = (_frameTime.QuadPart - _lastTime.QuadPart) 
		/ static_cast<float>(_cpuTick.QuadPart);

	_lastTime = _frameTime;

	return _timeDelta;
}

shared_ptr<CTimer> CTimer::Create()
{
	auto instance = make_shared<CTimer>();

	if (FAILED(instance->Ready_Timer()))
	{
		return nullptr;
	}

	return instance;
}

void CTimer::Free()
{
	CBase::Free();

}
