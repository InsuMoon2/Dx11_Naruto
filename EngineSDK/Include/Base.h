#pragma once

#include "Engine_Define.h"

NS_BEGIN(Engine)

class ENGINE_DLL Base abstract : public enable_shared_from_this<Base>
{
protected:
	Base();
	virtual ~Base() = default;

public:
	void Set_Name(const wstring& name) { _name = name; }
	const wstring& Get_Name() const { return _name; }

public:
	virtual void Free() { }

protected:
	wstring _name;

};

NS_END
