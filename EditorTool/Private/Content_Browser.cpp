#include "pch.h"
#include "Content_Browser.h"
#include "GameInstance.h"
#include "Texture.h"
#include "Utils.h"
#include "GameObject_Factory.h"

Content_Browser::Content_Browser()
    : EditorWindow(TEXT("Content Browser"))
{
}

Content_Browser::~Content_Browser()
{
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

    // 디폴트 프리팹 만들기
    Generate_Default_Prefabs();

    // 초기 리소스 스캔 Resources 폴더 기준
    Refresh_Resources();
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
        ImGui::InvisibleButton("vsplitter", ImVec2(4.f, ImGui::GetContentRegionAvail().y));
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
    _rootFolder = {};

    fs::path rootPath = TEXT("../../Client/Bin/Resources");

    if (fs::exists(rootPath))
    {
        Scan_Folder(rootPath, _rootFolder);
        _currentFolder = &_rootFolder;
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

    string id = "##" + Utils::ToString(node.fullPath);
    bool isOpen = ImGui::TreeNodeEx(id.c_str(), flags);

    ImGui::SameLine();

    if (_iconFolder && !_iconFolder->_SRVs.empty())
    {
        ImGui::Image((ImTextureID)_iconFolder->_SRVs[0].Get(), ImVec2(16.f, 16.f));
        ImGui::SameLine();
    }

    string folderName = Utils::ToString(node.name);
    ImGui::Text("%s", folderName.c_str());

    if (ImGui::IsItemClicked())
    {
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
    vector<wstring> validName = GameObject_Factory::Get_RegisteredNames();

    // 파일 목록 표시
    if (_currentFolder)
    {
        for (const auto& filePath : _currentFolder->files)
        {
            fs::path path(filePath);
            string fileName = path.filename().string();
            string extension = path.extension().string();
            string pureName = path.stem().string(); // 확장자 제거 (.json, .csv)

            // GameObject Factory에 등록된지 확인
            wstring pureNameW = Utils::ToWString(pureName);
            bool isValidPrefab = false;

            if (extension == ".json")
            {
                for (const auto& name : validName)
                {
                    if (name == pureNameW)
                    {
                        isValidPrefab = true;
                        break;
                    }
                }
            }

            // 아이콘
            ImGui::PushID(fileName.c_str());

            ImGui::Button(fileName.c_str(), ImVec2(_thumbnailSize, _thumbnailSize));

            if (isValidPrefab && ImGui::IsItemHovered && ImGui::IsMouseDoubleClicked(0))
            {
                // Prefab 인스턴스화 (수정 모드)
                auto newObj = GAME->Instantiate_Prefab(pureNameW, {});

                if (newObj)
                {
                    LOG_INFO("Prefab Opened for Edit : {}", pureName);
                }
            }

            if (isValidPrefab)
            {
                ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "%s", pureName.c_str());
            }
            else
            {
                ImGui::TextWrapped("%s", fileName.c_str());
            }

            ImGui::PopID();
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1); // 초기화

    // 빈 공간 우클릭 -> Create Prefab
    if (ImGui::BeginPopupContextWindow("AssetViewContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::BeginMenu("Create Prefab"))
        {
            // Factory에 등록된 이름 가져오기
            vector<wstring> names = GameObject_Factory::Get_RegisteredNames();

            for (const auto& nameW : names)
            {
                string typeName = Utils::ToString(nameW);

                if (ImGui::MenuItem(typeName.c_str()))
                {
                    // Create New Prefab
                    auto tempObj = GameObject_Factory::Create(nameW, GAME->Get_Device(), GAME->Get_Context());

                    if (tempObj)
                    {
                        // 현재 폴더에 json파일로 저장
                        string fileName = typeName + "_New.json";
                        fs::path savePath = fs::path(_currentFolder->fullPath) / fileName;

                        GAME->Save_Prefab(savePath.string(), tempObj);

                        Refresh_Resources();
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

}

void Content_Browser::Generate_Default_Prefabs()
{
    vector<wstring> names = GameObject_Factory::Get_RegisteredNames();

    fs::path prefabDir = TEXT("../../Client/Bin/Resources/Data/Prefabs");
    if (!fs::exists(prefabDir))
        fs::create_directory(prefabDir);

    int createCount = 0;

    for (const auto& nameW : names)
    {
        string typeName = Utils::ToString(nameW);
        string fileName = typeName + ".json";
        fs::path filePath = prefabDir / fileName;

        if (fs::exists(filePath))
            continue;

        auto tempObj = GameObject_Factory::Create(nameW, GAME->Get_Device(), GAME->Get_Context());
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

shared_ptr<Content_Browser> Content_Browser::Create()
{
    return make_shared<Content_Browser>();
}
