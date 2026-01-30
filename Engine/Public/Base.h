#pragma once

#include "Engine_Define.h"

NS_BEGIN(Engine)

class ENGINE_DLL Base abstract : public enable_shared_from_this<Base>
{
protected:
	Base();
	virtual ~Base() = default;

public:
	void SetName(const wstring& name) { _strName = name; }
	const wstring& GetName() const { return _strName; }

public:
	virtual void Free() { }

protected:
	wstring _strName;

};

NS_END
