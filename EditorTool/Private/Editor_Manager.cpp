#include "pch.h"
#include "EditorInstance.h"
#include "GameInstance.h"
#include "Editor_Manager.h"
#include "SceneView.h"
#include "HierarchyView.h"
#include "Console_View.h"
#include "Content_Browser.h"
#include "Inspector.h"
#include "Level_Serializer.h"
#include "PlayerSession_Manager.h"
#include "Profiler_View.h"
#include "RenderTarget.h"

Editor_Manager::~Editor_Manager()
{
}

void Editor_Manager::Initialize()
{
    Add_Window(TEXT("Scene"), SceneView::Create());
    Add_Window(TEXT("Hierarchy"), HierarchyView::Create());
    Add_Window(TEXT("Inspector"), Inspector::Create());

    Add_Window(TEXT("Console"), Console_View::Create());
    Add_Window(TEXT("Content Browser"), Content_Browser::Create());

    Add_Window(TEXT("Profiler"), Profiler_View::Create());

}

void Editor_Manager::Update(float timeDelta)
{
    Begin_DockSpace();
    Show_MenuBar();

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
    CHECK_NULL(renderTarget);

    renderTarget->BindAsTarget();
    renderTarget->Clear(Color(0.1f, 0.1f, 0.1f, 1.f));

    GAME->Draw();

    renderTarget->UnbindAll();
    GAME->BindBackBuffer();
}

void Editor_Manager::Add_Window(const wstring& key, shared_ptr<EditorWindow> window)
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

void Editor_Manager::Begin_DockSpace()
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
        float toolbarHeight = 34.f;

        int test;

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);

        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX((windowWidth / 2.f) - 150.f); // 왼쪽으로 이동

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

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_ELLIPSIS_VERTICAL "##PlayOptions"))
        {
            ImGui::OpenPopup("PlayOptionsPopup");
        }

        if (ImGui::BeginPopup("PlayOptionsPopup"))
        {
            ImGui::Text("Multiplayer Test");
            ImGui::Separator();

            if (ImGui::MenuItem("Single Player"))
            {
                EDITOR->Start_SinglePlayer();

                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("2 Players"))
            {
                EDITOR->Start_MultiPlayer(2);

                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("4 Players"))
            {
                EDITOR->Start_MultiPlayer(4);

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        float profilerButtonWidth = 100.f;
        float rightMargin = 280.f;

        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(windowWidth - profilerButtonWidth - rightMargin);

        if (ImGui::Button(ICON_FA_CHART_LINE " Profiler"))
        {
            auto profiler = dynamic_pointer_cast<Profiler_View>(Get_Window(TEXT("Profiler")));
            if (profiler)
                profiler->Set_Active(!profiler->IsActive());
        }
        ImGui::PopStyleVar();
     
        ImGui::SetCursorPosY(toolbarHeight);
    }
    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    //ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    //ImGui::DockSpaceOverViewport(dock_flags, ImGui::GetMainViewport());
}

void Editor_Manager::Show_MenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        // File
        if (ImGui::BeginMenu("File"))
        {
            if(ImGui::MenuItem("Save Level", "Ctrl+S"))
            {
                _showSaveLevelDialog = true;
                ::memset(_levelNameBuffer, 0, sizeof(_levelNameBuffer));
            }

            if (ImGui::MenuItem("Load Level", "Ctrl+O"))
            {
                _showLoadLevelDialog = true;
                _levelFiles.clear();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit"))
            {
                PostQuitMessage(0);
            }

            ImGui::EndMenu();
        }

        if (_showLoadLevelDialog)
            Show_LoadLevelDialog();

        if (_showSaveLevelDialog)
            Show_SaveLevelDialog();

        // 추가된 Viewer
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

void Editor_Manager::Show_SaveLevelDialog()
{
    if (!ImGui::IsPopupOpen("Save Level"))
        ImGui::OpenPopup("Save Level");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 150));

    if (ImGui::BeginPopupModal("Save Level", &_showSaveLevelDialog,
        ImGuiWindowFlags_NoResize))
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Enter level name:");
        ImGui::Spacing();

        // 텍스트 입력
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##LevelName", _levelNameBuffer, IM_ARRAYSIZE(_levelNameBuffer));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        bool canSave = (strlen(_levelNameBuffer) > 0);

        if (!canSave)
            ImGui::BeginDisabled();

        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            time_t now = time(nullptr);
            tm localTime;
            localtime_s(&localTime, &now);

            wchar_t dateStr[64];
            wcsftime(dateStr, 64, L"%Y%m%d", &localTime);

            string nameStr(_levelNameBuffer);

            wstring nameW(nameStr.begin(), nameStr.end());
            wstring fileName = L"[" + wstring(dateStr) + L"]" + nameW + L".level.json";

            On_SaveLevel(fileName);  

            _showSaveLevelDialog = false;
        }

        if (!canSave)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            _showSaveLevelDialog = false;
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
}

void Editor_Manager::Show_LoadLevelDialog()
{
    if (!ImGui::IsPopupOpen("Load Level"))
        ImGui::OpenPopup("Load Level");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 350));

    if (ImGui::BeginPopupModal("Load Level", &_showLoadLevelDialog,
        ImGuiWindowFlags_NoResize))
    {
        if (_levelFiles.empty())
            _levelFiles = Level_Serializer::Get_SaveFiles();

        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Select a level to load:");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginListBox("##LevelList", ImVec2(-1, 220)))
        {
            if (_levelFiles.empty())
            {
                ImGui::TextDisabled("No level files found in EditorTool/Data/Levels/");
            }
            else
            {
                for (int32 i = 0; i < _levelFiles.size(); i++)
                {
                    wstring& fileNameW = _levelFiles[i];
                    string fileName = string(fileNameW.begin(), fileNameW.end());

                    bool isSelected = (_selectedLevelIndex == i);
                    if (ImGui::Selectable(fileName.c_str(), isSelected))
                        _selectedLevelIndex = i;

                    // 더블 클릭 시 바로 로드
                    if (isSelected && ImGui::IsMouseDoubleClicked(0))
                    {
                        On_LoadLevel(_levelFiles[_selectedLevelIndex]);
                        _showLoadLevelDialog = false;
                        _selectedLevelIndex = -1;
                    }
                }
            }
            ImGui::EndListBox();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 버튼
        bool canLoad = (_selectedLevelIndex >= 0 && _selectedLevelIndex < _levelFiles.size());

        if (!canLoad)
            ImGui::BeginDisabled();

        if (ImGui::Button("Load", ImVec2(120, 0)))
        {
            On_LoadLevel(_levelFiles[_selectedLevelIndex]);
            _showLoadLevelDialog = false;
            _selectedLevelIndex = -1;
        }

        if (!canLoad)
            ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            _showLoadLevelDialog = false;
            _selectedLevelIndex = -1;
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
}

void Editor_Manager::On_SaveLevel(const wstring& fileName)
{
    uint32 currentLevel = GAME->Current_Level();
    auto objects = GAME->Get_GameObjects(currentLevel);
    Level_Serializer::Save_Level(fileName, objects);
}

void Editor_Manager::On_LoadLevel(const wstring& fileName)
{
    auto objects = Level_Serializer::Load_Level(fileName);

    // TODO : 
    // 1. 현재 레벨 클리어
    // 2. 로드한 오브젝트 추가
}

unique_ptr<Editor_Manager> Editor_Manager::Create()
{
    auto instance = make_unique<Editor_Manager>();

    instance->Initialize();

    return instance;
}
