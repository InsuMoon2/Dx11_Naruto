#include "pch.h"
#include "Converter.h"

int main()
{
    cout << "=== AssimpTool ===" << endl;

    auto converter = Converter::Create();

    //converter->ReadAssetFile(L"TestModel.fbx");

    cout << "=== Done ===" << endl;
}
