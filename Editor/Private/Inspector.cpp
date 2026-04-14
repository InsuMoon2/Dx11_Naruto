#include "pch.h"
#include "Inspector.h"
#include "GameObject.h"
#include "Reflection_Inspector.h"
#include "ContainerObject.h"
#include "PartObject.h"
#include "Transform.h"
#include "Inspector_Factory.h"
#include "Player.h"
#include "Customizer_Manager.h"

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

            auto container = dynamic_pointer_cast<ContainerObject>(_targetObject);
            if (container)
            {
                Draw_PartObjects(container);
                Draw_SelectedPartObject(container);
            }
        }

    }
    ImGui::End();
}

void Inspector::Draw_PartObjects(Shared<ContainerObject> container)
{
    if (!container)
        return;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text(ICON_FA_CUBES " Part Objects");
    ImGui::Spacing();

    for (auto slot : magic_enum::enum_values<ContainerObject::EPartSlot>())
    {
        if (slot == ContainerObject::EPartSlot::END)
            continue;

        auto part = container->Get_PartObject(slot);
        const bool exists = (part != nullptr);

        if (!exists)
            ImGui::BeginDisabled();

        const string slotName = ContainerObject::Get_PartSlotName(slot);
        const bool isSelected = (_selectedPartSlot == static_cast<int32>(slot));

        if (ImGui::Selectable(slotName.c_str(), isSelected))
            _selectedPartSlot = static_cast<int32>(slot);

        if (!exists)
            ImGui::EndDisabled();
    }
}

void Inspector::Draw_SelectedPartObject(Shared<ContainerObject> container)
{
    if (!container)
        return;

    if (_selectedPartSlot < 0 || _selectedPartSlot >= ETOI(ContainerObject::EPartSlot::END))
        return;

    const auto slot = static_cast<ContainerObject::EPartSlot>(_selectedPartSlot);
    const auto part = container->Get_PartObject(slot);
    if (!part)
        return;

    const auto transform = part->Get_Component<Transform>();
    if (!transform)
        return;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.f, 1.f), "[ Part Object ]");
    ImGui::Separator();
    ImGui::Text("Slot : %s", ContainerObject::Get_PartSlotName(slot).c_str());
    ImGui::Text("Class : %s", Utils::ToString(part->Get_Name()).c_str());
    ImGui::Spacing();

    const json beforeTransform = transform->To_Json();
    ImGui::PushID(ContainerObject::Get_PartSlotName(slot).c_str());
    Draw_Component(Transform::StaticTypeID(), transform);
    ImGui::PopID();

    const json afterTransform = transform->To_Json();
    if (beforeTransform != afterTransform)
    {
        container->Set_PartTransformOverride(slot, afterTransform);

        auto player = dynamic_pointer_cast<Client::Player>(_targetObject);
        if (player)
            GET_SINGLE(Client::Customizer_Manager)->Set_PartTransform(slot, afterTransform);

        part->Update(0.f);
        part->Late_Update(0.f);
    }

    for (auto& [id, comp] : part->Get_Components())
    {
        if (id == Transform::StaticTypeID() || !comp)
            continue;

        if (Inspector_Factory::GetInstance()->Get_Inspector(id) ||
            Inspector_Factory::GetInstance()->Get_Inspector_ByType(comp))
        {
            ImGui::Spacing();
            ImGui::PushID(id);
            Draw_Component(id, comp);
            ImGui::PopID();
        }
    }

    ImGui::Spacing();

    if (ImGui::Button("Reset Local Transform", ImVec2(-1.f, 28.f)))
    {
        transform->Set_LocalPosition(Vec3::Zero);
        transform->Set_LocalEulerAngles(0.f, 0.f, 0.f);
        transform->Set_LocalScale(1.f, 1.f, 1.f);

        const json resetTransform = transform->To_Json();
        container->Set_PartTransformOverride(slot, resetTransform);

        auto player = dynamic_pointer_cast<Client::Player>(_targetObject);
        if (player)
            GET_SINGLE(Client::Customizer_Manager)->Set_PartTransform(slot, resetTransform);

        part->Update(0.f);
        part->Late_Update(0.f);
    }
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
