#include "pch.h"
#include "Texture_Inspector.h"
#include "Component.h"
#include "Texture.h"

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

    ImGui::Text("File");
    ImGui::SameLine(80.f);
    ImGui::TextWrapped("%s", fileName.c_str());

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
    int currentIndex = static_cast<int>(texture->Get_CurrentIndex());
    int maxIndex = static_cast<int>(srvs.size()) - 1;

    if (ImGui::SliderInt("Frame", &currentIndex, 0, maxIndex))
    {
        texture->Set_CurrentIndex(static_cast<uint32>(currentIndex));
    }

    if (!srvs.empty())
    {
        ImGui::Spacing();
        ImGui::Text("Preview (%d / %d)", min(1, (int)srvs.size()), (int)srvs.size());

        ImGui::Image((ImTextureID)srvs[0].Get(), ImVec2(64, 64));

        for (uint32 i = 1; i < srvs.size() && i < 8; ++i)
        {
            ImGui::SameLine();
            ImGui::Image((ImTextureID)srvs[i].Get(), ImVec2(64, 64));
        }
    }
}
