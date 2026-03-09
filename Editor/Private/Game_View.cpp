#include "pch.h"
#include "Game_View.h"
#include "GameInstance.h"
#include "RenderTarget.h"

Game_View::Game_View()
    : EditorWindow(TEXT("Game"))
{
}

Game_View::~Game_View()
{
}

void Game_View::Initialize()
{
    EditorWindow::Initialize();

    _renderTarget = RenderTarget::Create(GAME->Get_Device(), GAME->Get_ViewportWidth(), GAME->Get_ViewportHeight());
}

void Game_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);
}

void Game_View::OnGui()
{
    ImGuiWindowFlags flag = Get_WindowFlags();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    string str = Utils::ToString(Get_Name());
    ImGui::Begin(str.c_str(), nullptr, flag);
    {
        Update_WindowState();

        Render_Viewport();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

ImGuiWindowFlags Game_View::Get_WindowFlags() const
{

    return ImGuiWindowFlags_None;
}

void Game_View::Update_WindowState()
{
    _isFocused = ImGui::IsWindowFocused();
    _isHovered = ImGui::IsWindowHovered();
}

void Game_View::Render_Viewport()
{
    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    _viewportSize = Vec2(panelSize.x, panelSize.y);

    // 임시: SceneView와 동일한 방식 사용 (추후 렌더타겟 분리 필요)
    // 지금은 SceneView의 RT를 가져올 수 없으므로, 이미지를 못 그림.
    // 일단 텍스트로 표시

    if (_renderTarget && panelSize.x > 0 && panelSize.y > 0)
    {
        _renderTarget->Resize(static_cast<uint32>(panelSize.x), static_cast<uint32>(panelSize.y));

        ImGui::Image((ImTextureID)_renderTarget->Get_SRV(), panelSize);
    }

    if (GAME->Get_GameState() != EGameState::Play)
    {
        ImVec2 textSize = ImGui::CalcTextSize("Press Play Button");
        ImGui::SetCursorPos(ImVec2((panelSize.x - textSize.x) * 0.5f, (panelSize.y - textSize.y) * 0.5f));
        ImGui::Text("Press Play Button");
    }
}

shared_ptr<Game_View> Game_View::Create()
{
    return make_shared<Game_View>();
}
