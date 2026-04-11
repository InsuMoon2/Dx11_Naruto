#include "pch.h"
#include "Converter.h"

namespace fs = std::filesystem;

static Assimp::EConvertModelType ParseConvertType(int argc, wchar_t** argv)
{
    if (argc < 4)
        return Assimp::EConvertModelType::Auto;

    string arg = fs::path(argv[3]).string();
    std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

    if (arg == "auto")
        return Assimp::EConvertModelType::Auto;

    if (arg == "static")
        return Assimp::EConvertModelType::StaticMesh;

    if (arg == "skeletal")
        return Assimp::EConvertModelType::SkeletalMesh;

    wcout << L"[AssimpTool] Unknown type option. Fallback to Auto: " << argv[3] << endl;
    return Assimp::EConvertModelType::Auto;
}

static fs::path FindExistingPathFromAncestors(const fs::path& start, const fs::path& relativePath)
{
    if (start.empty())
        return {};

    fs::path current = fs::absolute(start);
    if (!fs::is_directory(current))
        current = current.parent_path();

    while (!current.empty())
    {
        const fs::path candidate = current / relativePath;
        if (fs::exists(candidate))
            return fs::absolute(candidate);

        const fs::path parent = current.parent_path();
        if (parent == current)
            break;

        current = parent;
    }

    return {};
}

static fs::path ResolveDefaultMeshDir(const wchar_t* executablePath)
{
    static constexpr wchar_t kDefaultRelativePath[] =
        //LR"(Client\Bin\Resources\StaticMesh\KonohaVillage03\Meshes)";
        //LR"(Client\Bin\Resources\Models\Saske)";
        //LR"(Client\Bin\Resources\Models\WhiteZetsu)";
        //LR"(Client\Bin\Resources\Models\Custom\Animation)";
        //LR"(Client\Bin\Resources\Models\Weapon\BigSword)";
        //LR"(Client\Bin\Resources\Skills\RasenShuriken\Meshes)";
        LR"(Client\Bin\Resources\Skills\ETC\Meshes\Stone)";

    if (fs::path fromWorkingDir = FindExistingPathFromAncestors(fs::current_path(), kDefaultRelativePath);
        !fromWorkingDir.empty())
    {
        return fromWorkingDir;
    }

    if (executablePath != nullptr)
    {
        if (fs::path fromExecutableDir = FindExistingPathFromAncestors(fs::path(executablePath).parent_path(), kDefaultRelativePath);
            !fromExecutableDir.empty())
        {
            return fromExecutableDir;
        }
    }

    return fs::absolute(fs::path(kDefaultRelativePath));
}

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
    Assimp::EConvertModelType requestedType = Assimp::EConvertModelType::Auto;

    if (argc >= 3)
    {
        srcDir = fs::absolute(argv[1]);
        dstDir = fs::absolute(argv[2]);
        requestedType = ParseConvertType(argc, argv);
    }
    else
    {
        srcDir = ResolveDefaultMeshDir(argc > 0 ? argv[0] : nullptr);
        dstDir = srcDir;
        requestedType = Assimp::EConvertModelType::Auto;

        wcout << L"[AssimpTool] No arguments provided. Using default mesh path." << endl;
        wcout << L"  src: " << srcDir << endl;
        wcout << L"  dst: " << dstDir << endl;
        wcout << L"  type: Auto" << endl;
    }

    if (!fs::exists(srcDir))
    {
        wcerr << L"Source directory not found: " << srcDir << endl;
        return 1;
    }

    auto converter = Assimp::Converter::Create();
    int successCount = 0;
    int failCount = 0;

    for (const auto& entry : fs::recursive_directory_iterator(srcDir))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path srcPath = entry.path();
        if (!IsTargetMesh(srcPath))
            continue;

        const fs::path relStem = fs::relative(srcPath, srcDir).replace_extension("");
        const fs::path dstBase = dstDir / relStem;

        fs::create_directories(dstBase.parent_path());

        const bool ok = converter->Convert(
            srcPath.wstring(),
            dstBase.wstring(),
            requestedType);

        if (ok)
            ++successCount;
        else
            ++failCount;
    }

    wcout << L"Convert finished. success=" << successCount
        << L", fail=" << failCount << endl;

    return (failCount == 0) ? 0 : 2;
}
