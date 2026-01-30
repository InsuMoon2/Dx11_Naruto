#include "pch.h"
#include "EditorInstance.h"
#include "GameInstance.h"
#include "Editor_Manager.h"
#include "SceneView.h"
#include "HierarchyView.h"
#include "Logger.h"
#include "Content_Browser.h"
#include "Inspector.h"
#include "RenderTarget.h"

Editor_Manager::~Editor_Manager()
{
}

void Editor_Manager::Initialize()
{
    AddWindow(TEXT("Scene"), SceneView::Create());
    AddWindow(TEXT("Hierarchy"), HierarchyView::Create());
    AddWindow(TEXT("Inspector"), Inspector::Create());

    AddWindow(TEXT("Console"), Logger::Create());
    AddWindow(TEXT("Content Browser"), Content_Browser::Create());

}

void Editor_Manager::Update(float timeDelta)
{
    BeginDockSpace();
    ShowMenuBar();

    for (auto& [key, window] : _windows)
    {
        if (window && window->IsActive())
        {
            window->Update(timeDelta);
            window->OnGui();
        }
    }
}

void Editor_Manager::Render()
{
    auto sceneView = dynamic_pointer_cast<SceneView>(Get_Window(TEXT("Scene")));
    if (!sceneView || !sceneView->IsActive())
        return;

    auto renderTarget = sceneView->Get_RenderTarget();
    NULL_CHECK(renderTarget);

    renderTarget->BindAsTarget();
    renderTarget->Clear(Color(0.1f, 0.1f, 0.1f, 1.f));

    GAME->Draw();

    renderTarget->UnbindAll();
    GAME->BindBackBuffer();
}

void Editor_Manager::AddWindow(const wstring& key, shared_ptr<EditorWindow> window)
{
    _windows[key] = window;

    if (window)
        window->Initialize();
}

shared_ptr<EditorWindow> Editor_Manager::Get_Window(const wstring& key)
{
    auto it = _windows.find(key);
    if (it != _windows.end())
        return it->second;

    return nullptr;
}

void Editor_Manager::BeginDockSpace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    float menuBarHeight = ImGui::GetFrameHeight();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - menuBarHeight));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags host_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##DockSpaceHost", nullptr, host_flags);
    ImGui::PopStyleVar(3);
    {
        float toolbarHeight = 30.f;

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);

        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX((windowWidth / 2.f) - 150.f); // 중앙 정렬

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));

        if (!EDITOR->IsPlaying())
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));

            if (ImGui::Button(ICON_FA_PLAY "##Play"))
                EDITOR->Play();

            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));

            if (ImGui::Button(ICON_FA_STOP "##Stop"))
                EDITOR->Stop();

            ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        // Pause
        if (ImGui::Button(ICON_FA_PAUSE "##Pause"))
            EDITOR->Pause();

        ImGui::SameLine();

        // Next Frame
        if (ImGui::Button(ICON_FA_FORWARD_STEP "##NextFrame"))
        {
            // TODO: 한 프레임만 진행
        }
        ImGui::PopStyleVar();
     
        ImGui::SetCursorPosY(toolbarHeight);
    }
    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    //ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    //ImGui::DockSpaceOverViewport(dock_flags, ImGui::GetMainViewport());
}

void Editor_Manager::ShowMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        // File
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
                PostQuitMessage(0);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            for (auto& [key, window] : _windows)
            {
                bool active = window->IsActive();

                string name = string(window->Get_Name().begin(), window->Get_Name().end());
                if (ImGui::MenuItem(name.c_str(), nullptr, active))
                {
                    window->Set_Active(!active);
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

unique_ptr<Editor_Manager> Editor_Manager::Create()
{
    auto instance = make_unique<Editor_Manager>();
    NULL_CHECK_RETURN(instance, nullptr);

    instance->Initialize();

    return instance;
}
