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

public:
    template <typename T>
    static T Max(const T& a, const T& b)
    {
        return (a < b) ? b : a;
    }

    template <typename T>
    static T Min(const T& a, const T& b)
    {
        return (b < a) ? b : a;
    }
};

NS_END
