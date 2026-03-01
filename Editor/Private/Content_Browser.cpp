#include "pch.h"
#include "Content_Browser.h"
#include "GameInstance.h"
#include "EditorInstance.h"
#include "Texture.h"
#include "Utils.h"
#include "GameObject_Factory.h"
#include "GameObject.h"
#include <fstream>
#include "Prefab_View.h"
#include "BehaviorTree_View.h"
#include "Asset_Manager.h"

Content_Browser::Content_Browser()
    : EditorWindow(TEXT("Content Browser"))
{
}

Content_Browser::~Content_Browser()
{
    // 에디터 종료 시 설정 저장
    Save_Settings();
}

void Content_Browser::Initialize()
{
    EditorWindow::Initialize();

    // Temp
    _iconFolder = Texture::Create(
        GAME->Get_Device(),
        GAME->Get_Context(),
        TEXT("../../Client/Bin/Resources/Textures/Folder_Icon.png"), 1
    );

    // TODO : 디폴트 프리팹 만들기 -> 추후 삭제 예정. 굳이 필요 없을듯
    Generate_Default_Prefabs();

    // 초기 리소스 스캔 Resources 폴더 기준
    Refresh_Resources();

    // 경로 로드
    Load_Settings();

}

void Content_Browser::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    // 새로고침 - 필요할지? 일단 구현
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_F5))
    {
        Refresh_Resources();
    }
}

void Content_Browser::OnGui()
{
    string str = Utils::ToString(Get_Name());

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin(str.c_str(), nullptr, flags);
    {
        // 좌측 폴더 트리
        ImGui::BeginChild("FolderTree", ImVec2(_leftPanelWidth, 0), true);
        {
            // 검색바
            ImGui::InputTextWithHint("##Search", "검색...", _searchBuffer, IM_ARRAYSIZE(_searchBuffer));
            ImGui::Separator();

            // 트리 그리기
            Draw_FolderTree(_rootFolder);
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // [Splitter] 패널 크기 조절 바
        float availHeight = ImGui::GetContentRegionAvail().y;
        if (availHeight > 0.f)
        {
            ImGui::InvisibleButton("vsplitter", ImVec2(4.f, availHeight));
        }
        if (ImGui::IsItemActive())
        {
            _leftPanelWidth += ImGui::GetIO().MouseDelta.x;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        ImGui::SameLine();

        // 우측 패널, 파일 리스트 보여주기
        ImGui::BeginChild("에셋 뷰어", ImVec2(0.f, 0.f), true);
        {
            if (_currentFolder)
            {
                ImGui::Text("Path %s", Utils::ToString(_currentFolder->fullPath).c_str());

                float width = 100.f;
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - width);

                if (ImGui::Button(ICON_FA_ROTATE_RIGHT " 새로고침", ImVec2(width, 24)))
                {
                    Refresh_Resources();
                    GAME->Scan_Assets(TEXT("../../Client/Bin/Resources")); // Asset_Manager도 동기화
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("새로고침 [F5]");


                ImGui::Separator();
                Draw_AssetView();
            }
        }
        ImGui::EndChild();

    }
    ImGui::End();
}

void Content_Browser::Refresh_Resources()
{
    wstring savedPath = _currentFolder ? _currentFolder->fullPath : L"";
    _rootFolder = {};

    _noThumbnailGuids.clear();

    fs::path rootPath = TEXT("../../Client/Bin/Resources");

    if (fs::exists(rootPath))
    {
        Scan_Folder(rootPath, _rootFolder);

        if (!savedPath.empty())
        {
            FFolderNode* found = Find_FolderNode(_rootFolder, savedPath);
            _currentFolder = found ? found : &_rootFolder;
        }
        else
        {
            _currentFolder = &_rootFolder;
        }
    }
    
}

void Content_Browser::Refresh_CurrentFolder()
{
    if (!_currentFolder)
        return;

    _currentFolder->files.clear();

    for (const auto& entry : fs::directory_iterator(_currentFolder->fullPath))
    {
        if (!entry.is_directory() && entry.path().extension() != L".meta")
        {
            _currentFolder->files.emplace_back(entry.path());
        }
    }
}

void Content_Browser::Scan_Folder(const wstring& path, FFolderNode& node)
{
    node.fullPath = path;
    node.name = fs::path(path).filename().wstring();

    for (const auto& entry : fs::directory_iterator(path))
    {
        if (entry.is_directory())
        {
            FFolderNode child;
            Scan_Folder(entry.path(), child);
            node.subFolders.emplace_back(child);
        }
        else
        {
            if (entry.path().extension() != L".meta")
                node.files.emplace_back(entry.path());
        }
    }

}

void Content_Browser::Draw_FolderTree(FFolderNode& node)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (_currentFolder == &node)
        flags |= ImGuiTreeNodeFlags_Selected;

    // 리프 노드인지 확인 (하위폴더 확인)
    if (node.subFolders.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    // 펼쳐야 할 폴더면 강제로 열기
    if (_expandedFolders.contains(node.fullPath))
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        _expandedFolders.erase(node.fullPath);
    }

    string id = "##" + Utils::ToString(node.fullPath);
    bool isOpen = ImGui::TreeNodeEx(id.c_str(), flags);

    bool isClicked = ImGui::IsItemClicked();

    ImGui::SameLine();

    if (_iconFolder && !_iconFolder->Get_SRVs().empty())
    {
        ImGui::Image((ImTextureID)_iconFolder->Get_SRVs()[0].Get(), ImVec2(16.f, 16.f));
        ImGui::SameLine();
    }

    string folderName = Utils::ToString(node.name);

    ImGui::Text("%s", folderName.c_str());

    if (isClicked)
    {
        // 다른 폴더 클릭 시 기존 썸네일 캐시 비우기
        if (_currentFolder != &node)
            _thumbnailCache.clear();

        _currentFolder = &node;
    }
    if (isOpen)
    {
        for (auto& child : node.subFolders)
        {
            Draw_FolderTree(child);
        }
        ImGui::TreePop();
    }

}

void Content_Browser::Draw_AssetView()
{
    // 그리드 레이아웃
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(panelWidth / (_thumbnailSize + 20.f));
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, 0, false);

    // TODO : 매 프레임 찾기말고, 캐싱 해놓기 or 새로고침 버튼 하나 만들기?
    auto name = GAME->Get_RegisteredGameObjects();

    // 파일 목록 표시
    if (_currentFolder)
    {
        for (const auto& filePath : _currentFolder->files)
        {
            fs::path path(filePath);
            string fileName = path.filename().string();
            string extension = path.extension().string();

            size_t dotPos = fileName.find('.');
            string pureName = (dotPos != string::npos) ? fileName.substr(0, dotPos) : path.stem().string();

            string pathStr = Utils::ToString(filePath);

            bool isValidPrefab = pathStr.ends_with(".prefab.json");

            bool isBehaviorTree = pathStr.ends_with(".bt.json");

            // 아이콘
            ImGui::PushID(fileName.c_str());

            string guid = GAME->Find_AssetGUID(filePath);

            if (!guid.empty()
                && !_thumbnailCache.contains(guid)
                && !_noThumbnailGuids.contains(guid))
            {
                Load_Thumbnail(guid);
            }

            auto it = _thumbnailCache.find(guid);

            if (it != _thumbnailCache.end() && it->second)
            {
                ImGui::ImageButton(fileName.c_str(),
                    (ImTextureID)it->second.Get(),
                    ImVec2(_thumbnailSize, _thumbnailSize));
            }
            else
            {
                ImGui::Button(pureName.c_str(),
                    ImVec2(_thumbnailSize, _thumbnailSize));
            }

            // Prefab 드래그
            if (isValidPrefab && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
#pragma region Legacy : 경로 기반 Payload
                //wstring fullPath = filePath;
                //ImGui::SetDragDropPayload("CONTENT_PREFAB", fullPath.c_str(), (fullPath.size() + 1) * sizeof(wchar_t));
#pragma endregion

#pragma region GUID 기반 Payload
                string guid = GAME->Find_AssetGUID(filePath);
                ImGui::SetDragDropPayload("CONTENT_PREFAB", guid.c_str(), guid.size() + 1);
                ImGui::SetTooltip("%s", pureName.c_str());
#pragma endregion
                
                ImGui::EndDragDropSource();
            }

            // BehaviorTree 드래그
            else if (isBehaviorTree && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
#pragma region Legacy : 경로 기반 Payload
                //wstring fullPath = filePath;
                //ImGui::SetDragDropPayload("CONTENT_BEHAVIORTREE", fullPath.c_str(), (fullPath.size() + 1) * sizeof(wchar_t));
#pragma endregion

#pragma region GUID 기반 Payload
                string guid = GAME->Find_AssetGUID(filePath);
                ImGui::SetDragDropPayload("CONTENT_BEHAVIORTREE", guid.c_str(), guid.size() + 1);
                ImGui::SetTooltip("%s", pureName.c_str());
#pragma endregion
                
                ImGui::EndDragDropSource();
            }

            // 더블클릭 -> Prefab View 열기
            if (isValidPrefab && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                auto prefabView = dynamic_pointer_cast<Prefab_View>(EDITOR->Get_Window(TEXT("Prefab")));

                if (prefabView)
                {
                    string fullPath = Utils::ToString(filePath);

                    prefabView->Open_Prefab(pureName, fullPath);
                }
            }

            // 더블클릭 -> BehaviorTree View열기
            if (isBehaviorTree && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                auto behaviorView = dynamic_pointer_cast<BehaviorTree_View>(EDITOR->Get_Window(TEXT("BehaviorTree")));

                if (behaviorView)
                {
                    string fullPath = Utils::ToString(filePath);

                    behaviorView->Load_BehaviorTree(fullPath);
                }
            }

            if (_isRenaming && fs::absolute(_renamingFilePath) == fs::absolute(filePath))
            {
                if (_focusRenameInput)
                {
                    ImGui::SetKeyboardFocusHere();
                }

                ImGui::SetNextItemWidth(_thumbnailSize);
                if (ImGui::InputText("##Rename", _renameBuffer, IM_ARRAYSIZE(_renameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    // Enter 입력
                    Finish_Rename(filePath, _renameBuffer);
                }

                bool hasLostFocus = !_focusRenameInput && !ImGui::IsItemActive() && !ImGui::IsItemFocused();

                // Esc 또는 포커스 잃을 때 처리
                if (ImGui::IsKeyPressed(ImGuiKey_Escape) || hasLostFocus)
                {
                    _isRenaming = false;
                }
            }

            else
            {
                float textWidth = ImGui::CalcTextSize(pureName.c_str()).x;
                float columnWidth = _thumbnailSize;

                // 가운데 정렬
                float offset = (columnWidth - textWidth) * 0.5f;
                if (offset > 0)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                // 있다면, 초록색으로 세팅, 없다면 레거시 파일 새로고침 필요
                if (isValidPrefab || isBehaviorTree)
                {
                    ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "%s", pureName.c_str());
                }
                else
                {
                    ImGui::TextWrapped("%s", pureName.c_str());
                }
            }


            ImGui::PopID();
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1); // 초기화

    // 빈 공간 우클릭 -> Create Prefab
    if (ImGui::BeginPopupContextWindow("AssetViewContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::BeginMenu("프리펩 만들기"))
        {
            // Factory에 등록된 이름 가져오기
            auto names = GAME->Get_RegisteredGameObjects();

            for (const auto& [objectID, nameW] : names)
            {
                string typeName = Utils::ToString(nameW);

                if (ImGui::MenuItem(typeName.c_str()))
                {
                    Create_NewPrefab(objectID, nameW);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

}

void Content_Browser::Generate_Default_Prefabs()
{
    //vector<wstring> names = GameObject_Factory::GetInstance()->Get_RegisteredNames();
    auto names = GAME->Get_RegisteredGameObjects();

    fs::path prefabDir = TEXT("../../Client/Bin/Resources/Data/json/Prefabs");
    if (!fs::exists(prefabDir))
        fs::create_directory(prefabDir);

    int createCount = 0;

    for (const auto& [objectID, nameW] : names)
    {
        string typeName = Utils::ToString(nameW);
        string fileName = typeName + ".json";
        fs::path filePath = prefabDir / fileName;

        if (fs::exists(filePath))
            continue;

        auto tempObj = GameObject_Factory::GetInstance()->Create(nameW, GAME->Get_Device(), GAME->Get_Context());
        if (tempObj)
        {
            GAME->Save_Prefab(filePath.string(), tempObj);
            

            createCount++;
        }
    }

    if (createCount > 0)
    {
        LOG_WARN("Auto-generated {} Default Prefabs.", createCount);

        Refresh_Resources();
    }
}

void Content_Browser::Finish_Rename(const wstring& oldPath, const char* newName)
{
    fs::path oldFilePath(oldPath);
    string oldFileName = oldFilePath.filename().string();

    size_t firstDotIdx = oldFileName.find('.');
    string fullExtension = (firstDotIdx != string::npos) ? oldFileName.substr(firstDotIdx) : oldFilePath.extension().string();

    fs::path newFilePath = oldFilePath.parent_path() / (string(newName) + fullExtension);
    if (fs::exists(newFilePath))
    {
        LOG_WARN("이미 존재하는 파일명 입니다 : {}", newName);
        _isRenaming = false; return;
    }

    try
    {
        ifstream inFile(oldFilePath);
        json root;
        if (inFile.is_open()) {
            inFile >> root;
            inFile.close();

            if (root.contains("prefab_name"))
                root["prefab_name"] = newName;

            ofstream outFile(newFilePath);
            outFile << root.dump(4);
            outFile.close();
            fs::remove(oldFilePath);
        }
        else
        {
            fs::rename(oldFilePath, newFilePath);
        }

        fs::path oldMetaPath = oldFilePath.wstring() + L".meta";
        fs::path newMetaPath = newFilePath.wstring() + L".meta";

        if (fs::exists(oldMetaPath))
        {
            fs::rename(oldMetaPath, newMetaPath);
        }

        string guid = GAME->Find_AssetGUID(oldFilePath.wstring());
        if (!guid.empty())
        {
            GAME->Update_AssetPath(guid, newFilePath.wstring());
        }

        LOG_INFO("파일명 변경 완료: {} -> {}", oldFileName, newName);
        Refresh_CurrentFolder();
    }

    catch (const exception& e)
    {
        LOG_ERROR("파일명 변경 실패: {}", e.what());
    }
    _isRenaming = false;
}
void Content_Browser::Create_NewPrefab(uint32 objectID, const wstring& typeName)
{
    string baseFileName = "NewPrefab";
    fs::path savePath = fs::path(_currentFolder->fullPath) / (baseFileName + ".prefab.json");

    // 중복 이름 처리
    int counter = 1;
    while (fs::exists(savePath))
    {
        savePath = fs::path(_currentFolder->fullPath) / (baseFileName + "_" + to_string(counter++) + ".json");
    }

    auto tempObj = GAME->Clone_GameObject(0, objectID, nullptr);

    if (tempObj)
    {
        tempObj->Set_Name(typeName);
        GAME->Save_Prefab(savePath.string(), tempObj);

        _currentFolder->files.emplace_back(savePath.wstring());

        Enter_RenameMode(savePath);
    }
}

void Content_Browser::Enter_RenameMode(fs::path savePath)
{
    _isRenaming = true;
    _renamingFilePath = savePath.wstring();

    strcpy_s(_renameBuffer, savePath.stem().string().c_str());
    _focusRenameInput = true;
}

void Content_Browser::Save_Settings()
{
    json settings;
    settings["lastPath"] = Utils::ToString(_currentFolder ? _currentFolder->fullPath : L"");

    fs::path settingPath = CONTENT_BROWSER_PATH;

    if (!fs::exists(settingPath.parent_path()))
    {
        fs::create_directory(settingPath.parent_path());
    }

    ofstream file(settingPath);
    if (file.is_open())
    {
        file << settings.dump(4);
        file.close();
    }
}

void Content_Browser::Load_Settings()
{
    ifstream file(CONTENT_BROWSER_PATH);
    if (!file.is_open())
        return;

    json settings;
    file >> settings;
    file.close();

    if (settings.contains("lastPath"))
    {
        wstring lastPath = Utils::ToWString(settings["lastPath"].get<string>());

        // 해당 폴더 찾아서 설정
        FFolderNode* found = Find_FolderNode(_rootFolder, lastPath);
        if (found)
        {
            _currentFolder = found;
            Expand_PathTo(lastPath);
        }
    }

}

Content_Browser::FFolderNode* Content_Browser::Find_FolderNode(FFolderNode& node, const wstring& path)
{
    if (node.fullPath == path)
        return &node;

    for (auto& child : node.subFolders)
    {
        FFolderNode* found = Find_FolderNode(child, path);
        if (found)
        {
            return found;
        }
    }

    return nullptr;
}

void Content_Browser::Expand_PathTo(const wstring& targetPath)
{
    _expandedFolders.clear();

    fs::path current(targetPath);
    fs::path root(_rootFolder.fullPath);

    while (current != root && !current.empty())
    {
        _expandedFolders.insert(current.wstring());
        current = current.parent_path();
    }

    _expandedFolders.insert(root.wstring());
}

void Content_Browser::Load_Thumbnail(const string& guid)
{
    if (_thumbnailCache.contains(guid))
        return;
    
    if (_noThumbnailGuids.contains(guid))
        return;

    wstring absPath = GAME->Resolve_AssetPath(guid);
    bool isImage = false;
    if (!absPath.empty())
    {
        wstring ext = fs::path(absPath).extension().wstring();
        isImage = (ext == L".png" || ext == L".jpg" || ext == L".dds");
    }

    wstring loadPath;
    if (isImage)
    {
        loadPath = absPath;
    }
    else
    {
        fs::path thumbPath = fs::path("../../Client/Bin/Resources/Thumbnails") / (guid + ".png");
        if (fs::exists(thumbPath))
            loadPath = thumbPath.wstring();
    }

    if (loadPath.empty())
    {
        _noThumbnailGuids.insert(guid);
        return;
    }

    auto texture = Texture::Create(
        GAME->Get_Device(), GAME->Get_Context(), loadPath, 1);

    if (texture && !texture->Get_SRVs().empty())
        _thumbnailCache[guid] = texture->Get_SRVs()[0];

    else
        _noThumbnailGuids.insert(guid);
}

shared_ptr<Content_Browser> Content_Browser::Create()
{
    return make_shared<Content_Browser>();
}
