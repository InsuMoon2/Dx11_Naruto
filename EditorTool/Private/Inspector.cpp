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
            string name = Utils::ToString(_targetObject->Get_Name());
            ImGui::Text("Name: %s", name.c_str());
            ImGui::Separator();

            for (auto& [id, comp] : _targetObject->Get_Components())
            {
                if (comp == nullptr)
                    continue;

                Draw_Component(id, comp);
            }
        }

    }
    ImGui::End();
}

void Inspector::Draw_Component(uint32 id, Shared<Component> component)
{
}

void Inspector::Draw_CombatStat()
{
}

void Inspector::Draw_Transform(Shared<Transform> transform)
{
    CHECK_NULL(transform);

    // Position
    ImGui::SeparatorText("Position");

    Vec3 position = transform->Get_LocalPosition();
    float pos[3] = { position.x, position.y, position.z };

    if (ImGui::DragFloat3("##Position", pos, 0.1f))
    {
        transform->Set_LocalPosition(Vec3(pos[0], pos[1], pos[2]));
    }

    // Rotation
    ImGui::SeparatorText("Rotation");

    Vec3 eulerRad = transform->Get_LocalEulerAngles();
    Vec3 eulerDeg = eulerRad * (180.f / XM_PI);
    float rot[3] = { eulerDeg.x, eulerDeg.y, eulerDeg.z };

    if(ImGui::DragFloat3("##Rotation", rot, 1.0f))
    {
        Vec3 newEulerRad = Vec3(rot[0], rot[1], rot[2]) * (XM_PI / 180.f);
        transform->Set_LocalEulerAngles(newEulerRad.x, newEulerRad.y, newEulerRad.z);
    }

    // Scale
    ImGui::SeparatorText("Scale");

    Vec3 scale = transform->Get_LocalScale();
    float scl[3] = { scale.x, scale.y, scale.z };
    if (ImGui::DragFloat3("##Scale", scl, 0.1f))
    {
        transform->Set_LocalScale(Vec3(scl[0], scl[1], scl[2]));
    }
}

shared_ptr<Inspector> Inspector::Create()
{
    return make_shared<Inspector>();
}
