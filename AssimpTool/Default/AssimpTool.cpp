#include "pch.h"
#include "Converter.h"

int main()
{
    cout << "=== AssimpTool ===" << endl;

    auto converter = Converter::Create();

    const bool gaaraCheck = converter->Convert(
        L"../../Client/Bin/Resources/Models/Gaara/SK_CHR_Gaara.fbx",
        L"../../Client/Bin/Resources/Models/Gaara/SK_CHR_Gaara",
        Assimp::EConvertModelType::SkeletalMesh);

    const bool borutoCheck = converter->Convert(
        L"../../Client/Bin/Resources/Models/Boruto/Boruto.fbx",
        L"../../Client/Bin/Resources/Models/Boruto/Boruto",
        Assimp::EConvertModelType::SkeletalMesh);

    if (!gaaraCheck || !borutoCheck)
    {
        cout << "Convert Failed" << endl;
        return 1;
    }

    cout << "=== Finish ===" << endl;
    return 0;
}
