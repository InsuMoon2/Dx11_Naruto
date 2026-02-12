#include "pch.h"
#include "GameInstance.h"
#include "EditorInstance.h"
#include "Editor_Manager.h"
#include "BehaviorTree_View.h"
#include "Scene_View.h"
#include "Hierarchy.h"
#include "Console_View.h"
#include "Content_Browser.h"
#include "Inspector.h"
#include "Level_Serializer.h"
#include "PlayerSession_Manager.h"
#include "Profiler_View.h"
#include "RenderTarget.h"
#include "Game_View.h"
#include "Notification_Manager.h"
#include "Prefab_View.h"
#include "Scene_View.h"
#include "RenderTarget.h"

Editor_Manager::~Editor_Manager()
{
}

void Editor_Manager::Initialize()
{
    Add_Window(TEXT("Scene"), Scene_View::Create());
    Add_Window(TEXT("Game"), Game_View::Create());
    Add_Window(TEXT("Hierarchy"), Hierarchy::Create());
    Add_Window(TEXT("Inspector"), Inspector::Create());

    Add_Window(TEXT("Console"), Console_View::Create());
    Add_Window(TEXT("Content Browser"), Content_Browser::Create());

    Add_Window(TEXT("Profiler"), Profiler_View::Create());
    Add_Window(TEXT("Prefab"), Prefab_View::Create());

    Add_Window(TEXT("Behavior"), BehaviorTree_View::Create());
}

void Editor_Manager::Update(float timeDelta)
{
    Begin_DockSpace();
    Show_MenuBar();

    Handle_Shortcuts();

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
    Shared<RenderTarget> targetRT = nullptr;

    if (GAME->Get_GameState() == EGameState::Play)
    {
        auto gameView = dynamic_pointer_cast<Game_View>(Get_Window(TEXT("Game")));
        if (gameView && gameView->IsActive())
            targetRT = gameView->Get_RenderTarget();
    }
    else // Edit Mode
    {
        auto sceneView = dynamic_pointer_cast<Scene_View>(Get_Window(TEXT("Scene")));
        if (sceneView && sceneView->IsActive())
            targetRT = sceneView->Get_RenderTarget();
    }
    // 렌더링 수행
    if (targetRT)
    {
        targetRT->BindAsTarget();
        targetRT->Clear(Color(0.1f, 0.1f, 0.1f, 1.f));
        GAME->Draw(); // 실제 게임 렌더링
        targetRT->UnbindAll();
    }

    GAME->BindBackBuffer();
}

void Editor_Manager::Add_Window(const wstring& key, shared_ptr<EditorWindow> window)
{
    _windows[key] = window;

    if (window)
        window->Initialize();
}

void Editor_Manager::Handle_Shortcuts()
{
    bool ctrlPressed  = ImGui::GetIO().KeyCtrl;
    bool shiftPressed = ImGui::GetIO().KeyShift;

    if (ctrlPressed && ImGui::IsKeyPressed(ImGuiKey_S, false)) // false -> 반복 입력 방지
    {
        // Ctrl + Shift + S : 다른 이름으로 저장
        if (shiftPressed)
        {
            _showSaveLevelDialog = true;
            ::memset(_levelNameBuffer, 0, sizeof(_levelNameBuffer));
        }
        // Ctrl + S : 덮어쓰기
        else
        {
            if (!_lastLevelPath.empty())
            {
                On_SaveLevel(_lastLevelPath);
            }
            else
            {
                _showSaveLevelDialog = true;
                ::memset(_levelNameBuffer, 0, sizeof(_levelNameBuffer));
            }
        }
    }
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

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);

        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX((windowWidth / 2.f) - 150.f); // 왼쪽으로 이동

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));

        if (GAME->Get_GameState() == EGameState::Edit)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));

            if (ImGui::Button(ICON_FA_PLAY "##Play"))
            {
                ImGui::SetWindowFocus("Game");

                EDITOR->Play();
            }
            
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));

            if (ImGui::Button(ICON_FA_STOP "##Stop"))
            {
                ImGui::SetWindowFocus("Scene");

                EDITOR->Stop();
            }
                

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

    // 화면 중앙 
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 150));

    if (ImGui::BeginPopupModal("Save Level", &_showSaveLevelDialog, ImGuiWindowFlags_NoResize))
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Enter level name:");
        ImGui::Spacing();

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;

        bool isEnterPressed = false;

        // 포커스 세팅
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();

        // 텍스트 입력
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##LevelName", _levelNameBuffer, IM_ARRAYSIZE(_levelNameBuffer), flags))
            isEnterPressed = true;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ESC 종료
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            _showSaveLevelDialog = false;
            ImGui::CloseCurrentPopup();
        }

        float buttonWidth = 120.f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalButtonsWidth = (buttonWidth * 2.f) + spacing;

        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalButtonsWidth) * 0.5f);

        bool canSave = (strlen(_levelNameBuffer) > 0);
        if (!canSave) ImGui::BeginDisabled();

        if (ImGui::Button("Save", ImVec2(120, 0)) || (canSave && isEnterPressed))
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
    // 팝업이 열려있지 않다면 
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

        // 리스트 박스 시작
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

                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("Delete Level"))
                        {
                            _deleteTargetFile = fileNameW; // 삭제할 파일 기억
                            _showDeleteConfirm = true;     // 삭제 확인창 트리거
                        }
                        ImGui::EndPopup();
                    }
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

        if (_showDeleteConfirm)
            ImGui::OpenPopup("Delete Confirmation");

        Show_DeleteConfirmModal();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ESC 종료
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            _showLoadLevelDialog = false;
            ImGui::CloseCurrentPopup();
        }

        float buttonWidth = 120.f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalButtonsWidth = (buttonWidth * 2.f) + spacing;

        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - totalButtonsWidth) * 0.5f);

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

        // Cancel 버튼
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            _showLoadLevelDialog = false;
            _selectedLevelIndex = -1;
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
}

void Editor_Manager::Show_DeleteConfirmModal()
{
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Delete Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        string fileStr(_deleteTargetFile.begin(), _deleteTargetFile.end());
        ImGui::Text("Are you sure you want to delete this file?");
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", fileStr.c_str());
        ImGui::Separator();
        if (ImGui::Button("Delete", ImVec2(120, 0)))
        {
            wstring fullPath = Level_Serializer::Get_FullPath(_deleteTargetFile);

            // 실제 파일 삭제
            try
            {
                if (filesystem::exists(fullPath))
                {
                    filesystem::remove(fullPath);
                    LOG_WARN("File Deleted: {}", fileStr);
                }
            }
            catch (const exception& e)
            {
                LOG_ERROR("Delete Failed: {}", e.what());
            }

            // 리스트 갱신
            _levelFiles = Level_Serializer::Get_SaveFiles();
            _deleteTargetFile = L"";
            _showDeleteConfirm = false;

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            _deleteTargetFile = L"";
            _showDeleteConfirm = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Editor_Manager::On_SaveLevel(const wstring& fileName)
{
    Level_Serializer::Save_Level(fileName, GAME->Get_GameObjects(GAME->Current_Level()));
    _lastLevelPath = fileName; // 경로 갱신
    
    fs::path path(fileName);
    string pureName = path.stem().stem().string(); // 파일명 자르기

    LOG_WARN("Level Saved: {}", pureName);
    
    //NOTIFY("Level Saved");
}

void Editor_Manager::On_LoadLevel(const wstring& fileName)
{
    auto objects = Level_Serializer::Load_Level(fileName);

    // TODO : 
    // 1. 현재 레벨 클리어
    // 2. 로드한 오브젝트 추가

    _lastLevelPath = fileName;

    fs::path path(fileName);
    string pureName = path.stem().stem().string();

    LOG_WARN("Level Loaded: {}", pureName);
}

unique_ptr<Editor_Manager> Editor_Manager::Create()
{
    auto instance = make_unique<Editor_Manager>();

    instance->Initialize();

    return instance;
}
