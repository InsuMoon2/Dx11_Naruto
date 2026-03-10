#include "pch.h"
#include "Converter.h"

namespace fs = std::filesystem;

static bool IsTargetMesh(const fs::path& path)
{
    if (!path.has_extension())
        return false;

    string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext != ".gltf" && ext != ".fbx")
        return false;

    // 1차 작업에서는 충돌 메시와 하위 LOD를 제외
    const string name = path.stem().string();
    if (name.starts_with("COL_"))
        return false;
    if (name.find("_Lod") != string::npos)
        return false;

    return true;
}

int wmain(int argc, wchar_t** argv)
{
    fs::path srcDir;
    fs::path dstDir;

    if (argc >= 3)
    {
        srcDir = fs::absolute(argv[1]);
        dstDir = fs::absolute(argv[2]);
    }
    else
    {
        srcDir = fs::absolute(
            LR"(C:\Users\moon\Desktop\Jusin\GitDesktop\Dx11_Naruto\Client\Bin\Resources\StaticMesh\LobbyMapAssets\KonohaVilliage)");
        dstDir = srcDir;

        wcout << L"[AssimpTool] No arguments provided. Using default Konoha path." << endl;
        wcout << L"  src: " << srcDir << endl;
        wcout << L"  dst: " << dstDir << endl;
    }

    if (!fs::exists(srcDir))
    {
        wcerr << L"Source directory not found: " << srcDir << endl;
        return 1;
    }

    auto converter = Assimp::Converter::Create();
    int successCount = 0;
    int failCount = 0;

    for (const auto& entry : fs::directory_iterator(srcDir))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path srcPath = entry.path();
        if (!IsTargetMesh(srcPath))
            continue;

        const fs::path dstBase = dstDir / srcPath.stem();

        const bool ok = converter->Convert(
            srcPath.wstring(),
            dstBase.wstring(),
            Assimp::EConvertModelType::StaticMesh);

        if (ok)
            ++successCount;
        else
            ++failCount;
    }

    wcout << L"Convert finished. success=" << successCount
        << L", fail=" << failCount << endl;

    return (failCount == 0) ? 0 : 2;
}
