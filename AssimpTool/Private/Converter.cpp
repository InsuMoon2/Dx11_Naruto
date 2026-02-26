#include "pch.h"
#include "Converter.h"

Converter::Converter()
{
    _importer = make_shared<Assimp::Importer>();
}

void Converter::ReadAssetFile(const wstring& filePath)
{
    uint32 flag = { aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast };

    auto pathW = filesystem::path(filePath);

    string path = pathW.string();

    _scene = _importer->ReadFile(path, flag);
    if (_scene == nullptr)
    {
        LOG_INFO("Failed to Import : Asset Path : {}", path);
    }

}

unique_ptr<Converter> Converter::Create()
{
    return make_unique<Converter>();
}
