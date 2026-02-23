#include "pch.h"
#include "Inspector.h"
#include "GameObject.h"

Inspector::Inspector()
    : EditorWindow(TEXT("Inspector"))
{
}

Inspector::~Inspector()
{
}

void Inspector::Initialize()
{
    EditorWindow::Initialize();
}

void Inspector::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);
}

void Inspector::OnGui()
{
    string str = Utils::ToString(Get_Name());

    ImGui::Begin(str.c_str());
    {
        if (_targetObject != nullptr)
        {
            Draw_Components(_targetObject);
        }

    }
    ImGui::End();
}

void Inspector::Draw_Component(uint32 id, Shared<Component> component)
{
    auto inspector = Inspector_Factory::GetInstance()->Get_Inspector(id);

    if (!inspector)
        inspector = Inspector_Factory::GetInstance()->Get_Inspector_ByType(component);

    if (inspector)
    {
        inspector->Draw_Inspector(component);

        ImGui::Spacing();     
        ImGui::Separator();   
        ImGui::Spacing();     
    }

    else
    {
        //ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.2f, 1.0f));
        //ImGui::SeparatorText(fmt::format("Component Type: {}", id).c_str());
        //ImGui::PopStyleColor();
        //
        //json data = component->To_Json();
        //ImGui::TextWrapped("%s", data.dump(2).c_str());
    }

}

void Inspector::Draw_Components(Shared<GameObject> target)
{
    if (!target) return;

    string name = Utils::ToString(target->Get_Name());
    ImGui::Text("Name: %s", name.c_str());
    ImGui::Separator();

    for (auto& [id, comp] : target->Get_Components())
    {
        if (!comp) continue;
        Draw_Component(id, comp);
    }
}

shared_ptr<Inspector> Inspector::Create()
{
    return make_shared<Inspector>();
}
