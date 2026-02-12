#include "pch.h"
#include "Utils.h"

wstring Utils::ToWString(string value)
{
  return wstring(value.begin(), value.end());
}

string Utils::ToString(wstring value)
{
  return string(value.begin(), value.end());
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
