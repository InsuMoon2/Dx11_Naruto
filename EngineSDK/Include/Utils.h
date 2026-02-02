#pragma once

NS_BEGIN(Engine)

class ENGINE_DLL Utils
{
public:
    static wstring ToWString(string value);
    static string  ToString(wstring value);
};

NS_END
