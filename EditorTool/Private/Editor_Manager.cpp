#include "pch.h"
#include "EditorInstance.h"
#include "Editor_Manager.h"
#include "SceneView.h"
#include "HierarchyView.h"
#include "Logger.h"
#include "Content_Browser.h"
#include "Inspector.h"

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
        if (window)
        {
            window->OnGui();
        }
    }
}

void Editor_Manager::Render()
{
  
}

void Editor_Manager::AddWindow(const wstring& key, shared_ptr<EditorWindow> window)
{
    _windows[key] = window;

    if (window)
        window->Initialize();
}

void Editor_Manager::BeginDockSpace()
{
    ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpaceOverViewport(dock_flags, ImGui::GetMainViewport());
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

        // TODO : Temp Play 버튼
        float centerX = ImGui::GetWindowWidth() / 2.f;
        ImGui::SetCursorPosX(centerX - 40);

        if (EDITOR->IsPlaying())
        {
            if (ImGui::Button("||"))
                EDITOR->Pause();

            ImGui::SameLine();

            if (ImGui::Button("■"))
                EDITOR->Stop();
        }

        else
        {
            if (ImGui::Button("▶"))
                EDITOR->Play();
        }
        ImGui::EndMainMenuBar();
    }
}

unique_ptr<Editor_Manager> Editor_Manager::Create()
{
    auto instance = make_unique<Editor_Manager>();

    instance->Initialize();

    return instance;
}
