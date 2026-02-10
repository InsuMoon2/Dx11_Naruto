#include "pch.h"
#include "Level_Editor.h"

NS_BEGIN(EditorApp)

Level_Editor::Level_Editor(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level(device, context)
{
    
}

Level_Editor::~Level_Editor()
{
    
}

HRESULT Level_Editor::Initialize()
{

    // 에디터용 빈 레벨 - 아무것도 안 함
    // 씬은 File -> Load로 로드될 것

    return S_OK;
}

shared_ptr<Level_Editor> Level_Editor::Create(ComPtr<Device> device,
    ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Level_Editor>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_Editor");
        return nullptr;
    }

    return instance;
}

void Level_Editor::Free()
{
    Level::Free();
}

NS_END
