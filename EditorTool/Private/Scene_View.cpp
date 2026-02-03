#include "pch.h"
#include "Scene_View.h"
#include "RenderTarget.h"
#include "GameInstance.h"
#include "EditorInstance.h"
#include "Input_Manager.h"

Scene_View::Scene_View()
    : EditorWindow(TEXT("Scene"))
{
}

Scene_View::~Scene_View()
{
}

void Scene_View::Initialize()
{
    EditorWindow::Initialize();

    // RenderTarget 초기화
    _renderTarget = RenderTarget::Create(GAME->Get_Device(), GAME->Get_ViewportWidth(), GAME->Get_ViewportHeight());
}

void Scene_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    // TODO : 카메라 추가 후, 이동 세팅

    if (INPUT->KeyDown(KEY_TYPE::F11))
    {
        ToggleFullScreen();
    }

}

void Scene_View::OnGui()
{
    PrePare_Window();

    ImGuiWindowFlags flags = Get_WindowFlags();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    string str = Utils::ToString(Get_Name());
    ImGui::Begin(str.c_str(), nullptr, flags);
    {
        Update_WindowState();

        Render_Viewport();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

ImGuiWindowFlags Scene_View::Get_WindowFlags() const
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    if (_isFullScreen)
    {
        flags |= ImGuiWindowFlags_NoDecoration;
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoResize;
    }

    return flags;
}

void Scene_View::PrePare_Window()
{
    // DockID 복원
    if (_shouldRestoreWindow && _savedDockId != 0)
    {
        ImGui::SetNextWindowDockID(_savedDockId, ImGuiCond_Always);
        _shouldRestoreWindow = false;
    }

    // 전체화면 크기 설정
    if (_isFullScreen)
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    }
}

void Scene_View::Render_Viewport()
{
    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    _viewportSize = Vec2(panelSize.x, panelSize.y);

    if (_renderTarget && panelSize.x > 0 && panelSize.y > 0)
    {
        _renderTarget->Resize(static_cast<uint32>(panelSize.x),
            static_cast<uint32>(panelSize.y));

        ImGui::Image(_renderTarget->GetSRV(), panelSize);
    }
}

void Scene_View::Update_WindowState()
{
    if (!_isFullScreen)
        _savedDockId = ImGui::GetWindowDockID();

    _isFocused = ImGui::IsWindowFocused();
    _isHovered = ImGui::IsWindowHovered();
}

void Scene_View::ToggleFullScreen()
{
    _isFullScreen = !_isFullScreen;

    HWND hWnd = EDITOR->Get_WindowHandle();

    if (_isFullScreen)
    {
        // 현재 윈도우 크기/스타일 저장
        GetWindowRect(hWnd, &_windowedRect);
        _savedStyle = GetWindowLongPtr(hWnd, GWL_STYLE);

        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        SetWindowLongPtr(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, screenWidth, screenHeight,
            SWP_FRAMECHANGED);

        GAME->Resize_BackBuffer(screenWidth, screenHeight);
    }
    else
    {
        // 원래 윈도우 스타일/크기 복원
        SetWindowLongPtr(hWnd, GWL_STYLE, _savedStyle);
        SetWindowPos(hWnd, HWND_TOP,
            _windowedRect.left, _windowedRect.top,
            _windowedRect.right - _windowedRect.left,
            _windowedRect.bottom - _windowedRect.top,
            SWP_FRAMECHANGED);

        // Graphics 리사이즈
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        GAME->Resize_BackBuffer(clientRect.right, clientRect.bottom);

        _shouldRestoreWindow = true; 
    }
}


shared_ptr<Scene_View> Scene_View::Create()
{
    return make_shared<Scene_View>();
}
