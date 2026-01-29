#include "pch.h"
#include "EditorInstance.h"
#include "ImGui_Manager.h"    
#include "Editor_Manager.h"   

IMPLEMENT_SINGLETON(EditorInstance)

EditorInstance::~EditorInstance()
{
    Release();
}

HRESULT EditorInstance::Initialize_Editor(const EDITOR_DESC& desc, ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    _desc = desc;

    _imguiManager = ImGui_Manager::Create(desc.hWnd, device, context);
    NULL_CHECK_RETURN(_imguiManager, E_FAIL);

    _editorManager = Editor_Manager::Create();
    NULL_CHECK_RETURN(_editorManager, E_FAIL);


    return S_OK;
}

void EditorInstance::Update_Editor(float timeDelta)
{
    _imguiManager->Update(timeDelta);
    _editorManager->Update(timeDelta);
}

void EditorInstance::Render_Editor()
{
    _editorManager->Render();
    _imguiManager->Render();
}

void EditorInstance::Release()
{
    
}

void EditorInstance::Play()
{
}

void EditorInstance::Pause()
{
}

void EditorInstance::Stop()
{
}
