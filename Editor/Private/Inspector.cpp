#include "pch.h"
#include "Inspector.h"
#include "GameObject.h"
#include "Reflection_Inspector.h"

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
        // 컴포넌트가 DECLARE_REFLECTION()을 가지고 있는지 확인
        // 리플렉션 정보가 있으면 자동 렌더링
        auto& reflInfo = component->Get_ReflectionInfo();
        if (!reflInfo.properties.empty())
        {
            static Reflection_Inspector autoInspector;

            autoInspector.Draw_FromReflection(component.get(), reflInfo);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }
    }

}

void Inspector::Draw_Components(Shared<GameObject> target)
{
    if (!target) return;

    string name = Utils::ToString(target->Get_Name());
    ImGui::Text("Name: %s", name.c_str());
    ImGui::Separator();

    auto& refInfo = target->Get_ReflectionInfo();
    if (!refInfo.properties.empty())
    {
        static Reflection_Inspector autoInspector;
        autoInspector.Draw_FromReflection(target.get(), refInfo);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

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
