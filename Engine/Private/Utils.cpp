#include "pch.h"
#include "Utils.h"

wstring Utils::ToWString(string value)
{
    if (value.empty())
        return {};

    // Network/protobuf strings are stored as UTF-8 and must be restored as wide text for UI rendering.
    int32 convertedSize = MultiByteToWideChar(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int32>(value.size()),
        nullptr,
        0);

    // Legacy resource strings may still arrive in the local ANSI code page, so keep a fallback path.
    uint32 codePage = CP_UTF8;
    if (convertedSize <= 0)
    {
        codePage = CP_ACP;
        convertedSize = MultiByteToWideChar(
            codePage,
            0,
            value.c_str(),
            static_cast<int32>(value.size()),
            nullptr,
            0);
    }

    if (convertedSize <= 0)
        return {};

    // Converted wide result consumed by UI text and object names.
    wstring result(static_cast<size_t>(convertedSize), L'\0');
    MultiByteToWideChar(
        codePage,
        0,
        value.c_str(),
        static_cast<int32>(value.size()),
        result.data(),
        convertedSize);

    return result;
}

string Utils::ToString(wstring value)
{
    if (value.empty())
        return {};

    // Protobuf string fields require valid UTF-8, especially for Korean player names and chat labels.
    const int32 convertedSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int32>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (convertedSize <= 0)
        return {};

    // UTF-8 result passed to protobuf/json/string based runtime systems.
    string result(static_cast<size_t>(convertedSize), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.c_str(),
        static_cast<int32>(value.size()),
        result.data(),
        convertedSize,
        nullptr,
        nullptr);

    return result;
}

string Utils::EnumToString(uint32 id)
{
    return "";
}

string Utils::Generate_GUID()
{
    static uuids::uuid_system_generator gen {};
    uuids::uuid id = gen();

    return uuids::to_string(id);
}

string Utils::ToLowerCopy(string value)
{
    transform(value.begin(), value.end(), value.begin(), ::tolower);

    return value;
}

bool Utils::EndsWidth(const string& value, const string& suffix)
{
    if (value.size() < suffix.size())
        return false;

    return equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

json Utils::Vec3_ToJson(const Vec3 vec)
{
    return { vec.x, vec.y, vec.z };
}

Vec3 Utils::Vec3_FromJson(const json& j, const Vec3& fallback)
{
    if (!j.is_array() || j.size() < 3)
        return fallback;

    return Vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

json Utils::Quat_ToJson(const Quat& quat)
{
    return { quat.x, quat.y, quat.z, quat.w };
}

Quat Utils::Quat_FromJson(const json& j)
{
    if (!j.is_array() || j.size() < 4)
        return Quat::Identity;
    return Quat(j[0].get<float>(), j[1].get<float>(),
        j[2].get<float>(), j[3].get<float>());
}

Vec3 Utils::Safe_Normalize(const Vec3& value, const Vec3& fallback)
{
    Vec3 result = value;

    if (result.LengthSquared() <= FLT_EPSILON)
        return fallback;

    result.Normalize();

    return result;
}

Vec3 Utils::Project_OnPlane(const Vec3& value, const Vec3& planeNormal)
{
    Vec3 normal = Safe_Normalize(planeNormal, Vec3::Up);

    return value - normal * value.Dot(normal);
}

float Utils::RandomRange(float minValue, float maxValue)
{
    if (minValue > maxValue)
        std::swap(minValue, maxValue);

    const float ratio = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

    return minValue + (maxValue - minValue) * ratio;
}

Vec3 Utils::RandomDirection()
{
    Vec3 dir(
        RandomRange(-1.f, 1.f),
        RandomRange(-1.f, 1.f),
        RandomRange(-1.f, 1.f));

    if (dir.LengthSquared() < 0.0001f)
        dir = Vec3(0.f, 1.f, 0.f);

    dir.Normalize();

    return dir;
}
