#include "pch.h"
#include "Texture_Inspector.h"
#include "Component.h"
#include "Texture.h"
#include <shellapi.h>

void Texture_Inspector::Draw_Inspector(shared_ptr<Component> component)
{
    json data = component->To_Json();

    if (!Draw_Header("Texture"))
        return;

    string fullPath = data.value("texture_path", string("(none)"));

    // 파일명 추출
    string fileName = fs::path(fullPath).stem().string();

    size_t pos = fileName.find("%d");
    if (pos != string::npos)
        fileName = fileName.substr(0, pos);

    //ImGui::Text("File");
    //ImGui::SameLine(80.f);
    //ImGui::TextWrapped("%s", fileName.c_str());

    //int numSRVs = data.value("num_srvs", 1);
    //ImGui::Text("Count");
    //ImGui::SameLine(80.f);
    //ImGui::PushItemWidth(-1);

    //if (ImGui::InputInt("##NumSRVs", &numSRVs))
    //{
    //    numSRVs = max(1, numSRVs);
    //
    //    // 변경된 값으로 JSON 갱신 후 From_Json으로 적용
    //    data["num_srvs"] = static_cast<uint32>(numSRVs);
    //    component->From_Json(data);
    //}
    //ImGui::PopItemWidth();

    // 텍스쳐 미리보기
    auto texture = static_pointer_cast<Texture>(component);
    auto& srvs = texture->Get_SRVs();

    // 텍스처 인덱스 변경 추후에는 Model인지, 단일 Texture인지 판단하고 분기를 해줘야할거같다.
    //int currentIndex = static_cast<int>(texture->Get_CurrentIndex());
    //int maxIndex = static_cast<int>(srvs.size()) - 1;

    //ImGui::Text("Frame");
    //ImGui::SameLine(80.f);
    //ImGui::TextDisabled("%d / %d", currentIndex + 1, maxIndex + 1);

    if (!srvs.empty())
    {
        ImGui::Spacing();
        const int visibleCount = min(static_cast<int>(srvs.size()), 8);
        ImGui::Text("Preview (%d / %d)", visibleCount, static_cast<int>(srvs.size()));

        const float thumbSize = 64.f;
        const float spacing = ImGui::GetStyle().ItemSpacing.x;

        for (int i = 0; i < visibleCount; ++i)
        {
            ImGui::PushID(i);

            ImGui::ImageButton(
                "##TexturePreview",
                (ImTextureID)srvs[i].Get(),
                ImVec2(thumbSize, thumbSize));

            wstring srcPath = texture->Get_SourcePath(static_cast<uint32>(i));

            if (ImGui::IsItemHovered())
            {
                string displayName = srcPath.empty()
                    ? "(unknown)"
                    : fs::path(srcPath).filename().string();

                string displayPath = srcPath.empty()
                    ? "(no source path)"
                    : Utils::ToString(srcPath);

                ImGui::BeginTooltip();
                ImGui::Text("%s", displayName.c_str());
                ImGui::Separator();
                ImGui::TextDisabled("%s", displayPath.c_str());
                ImGui::EndTooltip();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (!srcPath.empty())
                {
                    wstring absPath = fs::absolute(srcPath).wstring();
                    wstring args = L"/select,\"" + absPath + L"\"";

                    ShellExecute(
                        nullptr,
                        L"open",
                        L"explorer.exe",
                        args.c_str(),
                        nullptr,
                        SW_SHOWNORMAL);
                }
            }

            ImGui::PopID();

            // 다음 아이템이 현재 줄에 들어갈 수 있을 때만 SameLine
            if (i + 1 < visibleCount)
            {
                float nextX = ImGui::GetItemRectMax().x + spacing + thumbSize;
                float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

                if (nextX <= windowVisibleX2)
                {
                    ImGui::SameLine();
                }
            }
        }
    }

}
