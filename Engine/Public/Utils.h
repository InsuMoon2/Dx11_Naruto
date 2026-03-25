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

    // Vec3 -> josn 배열 [x,y,z]
    static json Vec3_ToJson(const Vec3 vec);
    // json 배열 -> Vec3
    static Vec3 Vec3_FromJson(const json& j, const Vec3& fallback = Vec3::Zero);
    // Quat -> json 배열 [x,y,z,w]
    static json Quat_ToJson(const Quat& quat);
    // json 배열 -> Quat
    static Quat Quat_FromJson(const json& j);

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
