#pragma once

NS_BEGIN(Engine)

class ENGINE_DLL Utils
{
public:
    static wstring  ToWString(string value);
    static string   ToString(wstring value);

    static string   EnumToString(uint32 id);

    static string   Generate_GUID(); // GUID 생성

    static string   ToLowerCopy(string value);

    static bool     EndsWidth(const string& value, const string& suffix);

};

NS_END
