#pragma once

NS_BEGIN(Engine)

class ENGINE_DLL Utils
{
public:
    static wstring ToWString(string value);
    static string  ToString(wstring value);

    static string EnumToString(uint32 id);
};

NS_END
