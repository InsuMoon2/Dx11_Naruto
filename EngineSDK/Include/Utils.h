#pragma once

NS_BEGIN(Engine)

class ENGINE_DLL Utils
{
public:
    static wstring  ToWString(string value);
    static string   ToString(wstring value);

    static string   EnumToString(uint32 id);

    // GUID
    static string   Generate_GUID();

    static HRESULT  Save_To_File(const wstring& outputPath);

};

NS_END
