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
#include "Notification_Manager.h"

#include <shellapi.h>
#include "UI_Animation_View.h"
#include "Editor_Helper.h"

static const unordered_map<string, EIconType> g_ExtensionToIconMap =
{
    { ".csv", EIconType::Csv }, { ".CSV", EIconType::Csv },
    { ".json", EIconType::Json }, { ".JSON", EIconType::Json },
    { ".py", EIconType::Python }, { ".PY", EIconType::Python },
    { ".xlsx", EIconType::Xlsl}, { ".XLSX" , EIconType::Xlsl}
};

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

    _iconFiles.resize(ETOI(EIconType::END));

    _iconFiles[ETOI(EIconType::Folder)] = Texture::Create(
        GAME->Get_Device(),
        GAME->Get_Context(),
        TEXT("../../Client/Bin/Resources/Textures/Folder_Icon.png"), 1);

    _iconFiles[ETOI(EIconType::Csv)] = Texture::Create(
        GAME->Get_Device(),
        GAME->Get_Context(),
        TEXT("../../Client/Bin/Resources/Textures/DataTable.png"), 1);

    _iconFiles[ETOI(EIconType::Json)] = Texture::Create(
        GAME->Get_Device(),
        GAME->Get_Context(),
        TEXT("../../Client/Bin/Resources/Textures/JSON.png"), 1);

    _iconFiles[ETOI(EIconType::Python)] = Texture::Create(
        GAME->Get_Device(),
        GAME->Get_Context(),
        TEXT("../../Client/Bin/Resources/Textures/Python.png"), 1);

    _iconFiles[ETOI(EIconType::Xlsl)] = Texture::Create(
        GAME->Get_Device(),
        GAME->Get_Context(),
        TEXT("../../Client/Bin/Resources/Textures/DataTable.png"), 1);

    // 초기 리소스 스캔 Resources 폴더 기준
    Refresh_Resources();

    // 경로 로드
    Load_Settings();

}

void Content_Browser::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);

    // 새로고침 - 필요할지?
    //if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_F5))
    //{
    //    Refresh_Resources();
    //}
}

void Content_Browser::OnGui()
{
    string str = Utils::ToString(Get_Name());

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin(str.c_str(), nullptr, flags);
    {
        _isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        Handle_SideButtonEvnet();

        // 좌측 폴더 트리
        ImGui::BeginChild("FolderTree", ImVec2(_leftPanelWidth, 0), true);
        {
            // 검색바
            if (ImGui::InputTextWithHint("##Search", "검색...", _searchBuffer, IM_ARRAYSIZE(_searchBuffer)))
            {
                if (strlen(_searchBuffer) > 0)
                {
                    string searchLower = _searchBuffer;
                    transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

                    FFolderNode* firstMatch = Get_FirstMatchingFolder(_rootFolder, searchLower);
                    if (firstMatch && _currentFolder != firstMatch)
                    {
                        _thumbnailCache.clear();
                        _currentFolder = firstMatch;

                        Expand_PathTo(firstMatch->fullPath);
                    }
                }
            }

            ImGui::Separator();

            ImGui::BeginChild("FolderTreeScroll", ImVec2(0, 0), false);
            Draw_FolderTree(_rootFolder);
            ImGui::EndChild();

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
                ImGui::BeginDisabled(!Can_GoBack());
                if (ImGui::ArrowButton("##BackFolder", ImGuiDir_Left))
                {
                    Go_BackFolder();
                }
                ImGui::EndDisabled();

                ImGui::SameLine();

                ImGui::BeginDisabled(!Can_GoForward());
                if (ImGui::ArrowButton("##ForwardFolder", ImGuiDir_Right))
                {
                    Go_ForwardFolder();
                }
                ImGui::EndDisabled();

                ImGui::SameLine();

                string pathStr = Utils::ToString(_currentFolder->fullPath);
                char pathBuffer[512] = {};
                strncpy_s(pathBuffer, pathStr.c_str(), _TRUNCATE);

                float pathWidth = min(500.f, ImGui::GetContentRegionAvail().x - 220.f);
                if (pathWidth < 150.f)
                    pathWidth = 150.f;

                ImGui::SetNextItemWidth(pathWidth);
                ImGui::InputText("##CurrentPath", pathBuffer, IM_ARRAYSIZE(pathBuffer), ImGuiInputTextFlags_ReadOnly);

                float buttonWidth = 100.f;
                float clearWidth = 80.f;
                float right = ImGui::GetContentRegionAvail().x;

                ImGui::SameLine(right - (buttonWidth + 8.f + clearWidth));

                if (ImGui::Button(ICON_FA_ROTATE_RIGHT " 새로고침", ImVec2(buttonWidth, 24)))
                {
                    Refresh_Resources();
                    GAME->Scan_Assets(TEXT("../../Client/Bin/Resources")); // Asset_Manager도 동기화
                    NOTIFY("새로고침");
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("새로고침 [F5]");
                }

                ImGui::SameLine();

                if (ImGui::Button("Clear", ImVec2(clearWidth, 24)))
                {
                    const uint32 removed = GAME->Clear_DisallowedMeta(_currentFolder->fullPath);
                    Refresh_Resources();
                    NOTIFY(std::format("{}개의 불필요한 .meta 삭제", removed).c_str());
                }

                ImGui::Separator();

                ImGui::BeginChild("AssetViewScroll", ImVec2(0, 0), false);
                Draw_AssetView();
                ImGui::EndChild();
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

    if (_currentFolder && _folderHistory.empty())
    {
        Record_FolderHistory(_currentFolder->fullPath);
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
            FFileEntry fe;
            fe.filePath = entry.path().wstring();
            fe.guid = GAME->Find_AssetGUID(fe.filePath);

            _currentFolder->files.emplace_back(fe);
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
            {
                FFileEntry fe;
                fe.filePath = entry.path().wstring();
                fe.guid = GAME->Find_AssetGUID(fe.filePath);

                node.files.emplace_back(fe);
            }
                
        }
    }

}

void Content_Browser::Draw_FolderTree(FFolderNode& node)
{
    if (strlen(_searchBuffer) > 0 && _currentFolder != &node)
    {
        string searchLower = _searchBuffer;
        transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

        if (node.fullPath != _rootFolder.fullPath && !IsFolderMatchingSearch(node, searchLower))
            return;

        // 검색어가 포함된 폴더 경로라면 보기 편하게 트리를 강제로 펼쳐둠
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }


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

    Draw_FolderContextMenu(node, "FolderTileContext");

    bool isClicked = ImGui::IsItemClicked();

    ImGui::SameLine();

    if (_iconFiles[ETOI(EIconType::Folder)] && !_iconFiles[ETOI(EIconType::Folder)]->Get_SRVs().empty())
    {
        ImGui::Image((ImTextureID)_iconFiles[ETOI(EIconType::Folder)]->Get_SRVs()[0].Get(), ImVec2(16.f, 16.f));
        ImGui::SameLine();
    }

    string folderName = Utils::ToString(node.name);

    ImGui::Text("%s", folderName.c_str());

    if (isClicked)
    {
        Open_Folder(&node);
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

    // 파일 목록 표시
    if (_currentFolder)
    {
        for (auto& childFolder : _currentFolder->subFolders)
        {
            Draw_FolderTile(childFolder);
        }

        for (const auto& fileEntry : _currentFolder->files)
        {
            const wstring& filePath = fileEntry.filePath;
            const string& guid = fileEntry.guid;
            fs::path path(filePath);

            string fileName = path.filename().string();
            string extension = path.extension().string();

            size_t dotPos = fileName.find('.');
            string pureName = (dotPos != string::npos) ? fileName.substr(0, dotPos) : path.stem().string();

            string pathStr = Utils::ToString(filePath);

            EAssetOpenType assetType = Editor_Helper::ClassifyAsset(path);

            // 아이콘
            ImGui::PushID(fileName.c_str());

            if (!guid.empty()
                && !_thumbnailCache.contains(guid)
                && !_noThumbnailGuids.contains(guid))
            {
                Load_Thumbnail(guid);
            }

            auto it = _thumbnailCache.find(guid);

            if (it != _thumbnailCache.end() && it->second)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                {
                    ImGui::ImageButton(fileName.c_str(),
                        (ImTextureID)it->second.Get(),
                        ImVec2(_thumbnailSize, _thumbnailSize));
                }
                ImGui::PopStyleVar();

            }
            else
            {
                auto iconIt = g_ExtensionToIconMap.find(extension);

                if (iconIt != g_ExtensionToIconMap.end())
                {
                    EIconType targetIcon = iconIt->second;
                    auto& iconTexture = _iconFiles[ETOI(targetIcon)];

                    if (iconTexture && !iconTexture->Get_SRVs().empty())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                        ImGui::ImageButton(fileName.c_str(),
                            (ImTextureID)iconTexture->Get_SRVs()[0].Get(),
                            ImVec2(_thumbnailSize, _thumbnailSize));
                        ImGui::PopStyleVar();
                    }
                }
                else
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    {
                        ImGui::Button(pureName.c_str(),
                            ImVec2(_thumbnailSize, _thumbnailSize));
                    }
                    ImGui::PopStyleVar();
                }
            }

            Draw_FileContextMenu(filePath, "FileItemContext");

            const bool isFocused = ImGui::IsItemFocused();
            bool isSelected = (_selectedFilePath == filePath);

            if (isSelected)
            {
                ImVec2 itemMin = ImGui::GetItemRectMin();
                ImVec2 itemMax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddRect(
                    itemMin, itemMax,
                    IM_COL32(70, 130, 210, 255),
                    0.f,
                    0,
                    2.f);
            }

            if (isFocused && !isSelected)
            {
                _selectedFilePath = filePath;
            }

            if (isFocused && ImGui::IsKeyPressed(ImGuiKey_Enter))
            {
                if (!isSelected)
                {
                    _selectedFilePath = filePath;
                }
                else
                {
                    Editor_Helper::Open_Asset(assetType, filePath, pureName);
                }
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                _selectedFilePath = filePath;
            }

            if (isSelected && !_isRenaming && ImGui::IsKeyPressed(ImGuiKey_F2))
            {
                Enter_RenameMode(filePath);
            }

            // Prefab 드래그
            if (assetType == EAssetOpenType::Prefab && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                if (!guid.empty())
                {
                    ImGui::SetDragDropPayload("CONTENT_PREFAB", guid.c_str(), guid.size() + 1);
                    ImGui::SetTooltip("%s", pureName.c_str());
                }
                ImGui::EndDragDropSource();
            }
            // BehaviorTree 드래그
            else if (assetType == EAssetOpenType::BehaviorTree && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                if (!guid.empty())
                {
                    ImGui::SetDragDropPayload("CONTENT_BEHAVIORTREE", guid.c_str(), guid.size() + 1);
                    ImGui::SetTooltip("%s", pureName.c_str());
                }
                ImGui::EndDragDropSource();
            }
            // Static Mesh 드래그
            else if (assetType == EAssetOpenType::Mesh && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                if (!guid.empty())
                {
                    ImGui::SetDragDropPayload("CONTENT_MESH", guid.c_str(), guid.size() + 1);
                    ImGui::SetTooltip(ICON_FA_CUBE " %s", pureName.c_str());
                }
                ImGui::EndDragDropSource();
            }
            else if (assetType == EAssetOpenType::Texture && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                if (!guid.empty())
                {
                    ImGui::SetDragDropPayload("CONTENT_TEXTURE", guid.c_str(), guid.size() + 1);
                    ImGui::SetTooltip(ICON_FA_IMAGE " %s", pureName.c_str());
                }
                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                string fullPath = Utils::ToString(filePath);

                Editor_Helper::Open_Asset(assetType, filePath, pureName);
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
                if (Editor_Helper::IsEditorManagedAsset(assetType))
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

            sort(names.begin(), names.end(),
                [](const auto& lhs, const auto& rhs)
                {
                    return Utils::ToString(lhs.second) < Utils::ToString(rhs.second);
                });

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

void Content_Browser::Draw_FolderTile(FFolderNode& folder)
{
    ImGui::PushID(Utils::ToString(folder.fullPath).c_str());

    if (_iconFiles[ETOI(EIconType::Folder)] && !_iconFiles[ETOI(EIconType::Folder)]->Get_SRVs().empty())
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::ImageButton(
            "##FolderTile",
            (ImTextureID)_iconFiles[ETOI(EIconType::Folder)]->Get_SRVs()[0].Get(),
            ImVec2(_thumbnailSize, _thumbnailSize));
        ImGui::PopStyleVar();
    }
    else
    {
        ImGui::Button("Folder", ImVec2(_thumbnailSize, _thumbnailSize));
    }

    const bool isFocused = ImGui::IsItemFocused();
    const bool isSelected = (_selectedFilePath == folder.fullPath);

    if (isSelected)
    {
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRect(
            itemMin, itemMax,
            IM_COL32(70, 130, 210, 255),
            0.f,
            0,
            2.f);
    }

    if (isFocused && !isSelected)
    {
        _selectedFilePath = folder.fullPath;
    }

    if (isFocused && ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
        if (!isSelected)
        {
            _selectedFilePath = folder.fullPath;
        }
        else
        {
            Open_Folder(&folder);
        }
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
    {
        Open_Folder(&folder);
    }

    // 우클릭 메뉴
    Draw_FolderContextMenu(folder, "FolderTileContext");

    string folderName = Utils::ToString(folder.name);
    float textWidth = ImGui::CalcTextSize(folderName.c_str()).x;
    float columnWidth = _thumbnailSize;
    float offset = (columnWidth - textWidth) * 0.5f;

    if (offset > 0)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

    ImGui::TextWrapped("%s", folderName.c_str());

    ImGui::PopID();
    ImGui::NextColumn();
}

void Content_Browser::Open_Folder(FFolderNode* folder)
{
    if (!folder)
        return;

    if (_currentFolder != folder)
        _thumbnailCache.clear();

    _currentFolder = folder;
    _selectedFilePath.clear();

    Expand_PathTo(folder->fullPath);

    if (!_suppressHistoryRecord)
    {
        Record_FolderHistory(folder->fullPath);
    }
}

void Content_Browser::Draw_FolderContextMenu(FFolderNode& folder, const string& popupId)
{
    if (!ImGui::BeginPopupContextItem(popupId.c_str()))
        return;

    if (ImGui::MenuItem("탐색기에서 열기"))
    {
        wstring absPath = fs::absolute(folder.fullPath).wstring();

        ShellExecute(
            nullptr,
            L"open",
            absPath.c_str(),
            nullptr,
            nullptr,
            SW_SHOWNORMAL);
    }

    ImGui::EndPopup();
}

void Content_Browser::Draw_FileContextMenu(const wstring& filePath, const string& popupId)
{
    if (!ImGui::BeginPopupContextItem(popupId.c_str()))
        return;

    if (ImGui::MenuItem("탐색기에서 열기"))
    {
        wstring absPath = fs::absolute(filePath).wstring();
        wstring args = L"/select,\"" + absPath + L"\"";

        ShellExecute(
            nullptr,
            L"open",
            L"explorer.exe",
            args.c_str(),
            nullptr,
            SW_SHOWNORMAL);
    }

    ImGui::EndPopup();
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

bool Content_Browser::IsFolderMatchingSearch(const Content_Browser::FFolderNode& node, const string& searchStr)
{
    string nameLower = Utils::ToString(node.name);
    transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

    if (nameLower.find(searchStr) != string::npos)
        return true;

    for (const auto& child : node.subFolders)
    {
        if (IsFolderMatchingSearch(child, searchStr))
            return true;
    }
    return false;
}

Content_Browser::FFolderNode* Content_Browser::Get_FirstMatchingFolder(FFolderNode& node, const string& searchStr)
{
    string nameLower = Utils::ToString(node.name);
    transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

    if (nameLower.find(searchStr) != string::npos)
        return &node;

    for (auto& child : node.subFolders)
    {
        FFolderNode* found = Get_FirstMatchingFolder(child, searchStr);
        if (found)
        {
            return found;
        }
    }

    return nullptr;
}

void Content_Browser::Record_FolderHistory(const wstring& path)
{
    if (_folderHistoryIndex >= 0 && _folderHistoryIndex < static_cast<int>(_folderHistory.size()))
    {
        if (_folderHistory[_folderHistoryIndex] == path)
            return;
    }

    // 현재 위치 뒤의 foward 히스토리는 잘라내기
    if (_folderHistoryIndex + 1 < static_cast<int>(_folderHistory.size()))
    {
        _folderHistory.erase(_folderHistory.begin() + _folderHistoryIndex + 1, _folderHistory.end());
    }

    _folderHistory.push_back(path);
    _folderHistoryIndex = static_cast<int>(_folderHistory.size()) - 1;
}

bool Content_Browser::Can_GoBack() const
{
    return _folderHistoryIndex > 0;
}

bool Content_Browser::Can_GoForward() const
{
    return _folderHistoryIndex >= 0
        && _folderHistoryIndex + 1 < static_cast<int>(_folderHistory.size());
}

void Content_Browser::Go_BackFolder()
{
    if (!Can_GoBack())
        return;

    --_folderHistoryIndex;

    FFolderNode* target = Find_FolderNode(_rootFolder, _folderHistory[_folderHistoryIndex]);
    if (!target)
        return;

    _suppressHistoryRecord = true;
    Open_Folder(target);
    _suppressHistoryRecord = false;
}

void Content_Browser::Go_ForwardFolder()
{
    if (!Can_GoForward())
        return;

    ++_folderHistoryIndex;

    FFolderNode* target = Find_FolderNode(_rootFolder, _folderHistory[_folderHistoryIndex]);
    if (!target)
        return;

    _suppressHistoryRecord = true;
    Open_Folder(target);
    _suppressHistoryRecord = false;
}

void Content_Browser::Handle_SideButtonEvnet()
{
    const bool isBrowserFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    const bool isBrowserHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    if (isBrowserFocused || isBrowserHovered)
    {
        if (ImGui::IsMouseClicked(3))
        {
            Go_BackFolder();
        }

        if (ImGui::IsMouseClicked(4))
        {
            Go_ForwardFolder();
        }
    }

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
