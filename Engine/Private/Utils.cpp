#include "pch.h"
#include "Utils.h"
#include "Enum.pb.h"

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
