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
#include "AttachedEffectObject.h"
#include "EffectComponent.h"

// 런타임 이펙트 레이어 타입 이름을 인스펙터에 짧게 표시할 때 호출한다.
static string Get_EffectLayerKindLabel(Engine::EEffectLayerKind kind)
{
    const auto enumName = magic_enum::enum_name(kind);
    if (!enumName.empty())
        return string(enumName);

    return "Unknown";
}

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

    Draw_RuntimeEffectDebug(target);

    for (auto& [id, comp] : target->Get_Components())
    {
        if (!comp) continue;
        Draw_Component(id, comp);
    }
}

void Inspector::Draw_RuntimeEffectDebug(Shared<GameObject> target)
{
    if (!target)
        return;

    auto selfEffectCom = target->Get_Component<Engine::EffectComponent>();
    vector<Shared<Client::AttachedEffectObject>> attachedEffects;

    const auto objects = GAME->Get_GameObjects(GAME->Current_Level());
    attachedEffects.reserve(objects.size());

    for (auto& obj : objects)
    {
        auto attachedEffect = dynamic_pointer_cast<Client::AttachedEffectObject>(obj);
        if (!attachedEffect)
            continue;

        auto owner = attachedEffect->Get_Owner();
        if (!owner || owner.get() != target.get())
            continue;

        attachedEffects.push_back(attachedEffect);
    }

    if (!selfEffectCom && attachedEffects.empty())
        return;

    ImGui::TextColored(ImVec4(1.f, 0.85f, 0.35f, 1.f), ICON_FA_BOLT " Runtime Effects");
    ImGui::Separator();

    if (selfEffectCom)
    {
        const string currentAssetName = selfEffectCom->Get_CurrentAssetName().empty()
            ? "(None)"
            : selfEffectCom->Get_CurrentAssetName();
        const auto& activeLayers = selfEffectCom->Get_ActiveLayers();

        if (ImGui::TreeNodeEx("Self EffectComponent", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Asset : %s", currentAssetName.c_str());
            ImGui::Text("Playing : %s", selfEffectCom->Is_Playing() ? "true" : "false");
            ImGui::Text("LifeSpan : %.3f", selfEffectCom->Get_LifeSpan());
            ImGui::Text("Active Layers : %zu", activeLayers.size());

            for (size_t i = 0; i < activeLayers.size(); ++i)
            {
                const auto& layer = activeLayers[i];
                const string layerLabel = "[" + to_string(i) + "] " + layer.desc.base.layerName;

                if (ImGui::TreeNode(layerLabel.c_str()))
                {
                    const string kindLabel = Get_EffectLayerKindLabel(layer.desc.base.kind);
                    ImGui::Text("Kind : %s", kindLabel.c_str());
                    ImGui::Text("Started : %s", layer.started ? "true" : "false");
                    ImGui::Text("Finished : %s", layer.finished ? "true" : "false");
                    ImGui::Text("Elapsed : %.3f", layer.elapsed);
                    ImGui::Text("Start Delay : %.3f", layer.desc.base.startDelay);
                    ImGui::Text("Duration : %.3f", layer.desc.base.duration);
                    ImGui::Text("Loop : %s", layer.desc.base.loop ? "true" : "false");
                    ImGui::Text("Object Alive : %s", layer.obj ? "true" : "false");
                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }
    }

    const string attachedHeader = "Attached Effects (" + to_string(attachedEffects.size()) + ")";
    if (ImGui::TreeNodeEx(attachedHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (attachedEffects.empty())
        {
            ImGui::TextDisabled("(AttachedEffectObject 없음)");
        }
        else
        {
            for (size_t i = 0; i < attachedEffects.size(); ++i)
            {
                const auto& attachedEffect = attachedEffects[i];
                const string effectName = attachedEffect->Get_EffectAssetName().empty()
                    ? "(None)"
                    : attachedEffect->Get_EffectAssetName();
                const string rowLabel = "[" + to_string(i) + "] " + effectName;
                auto effectCom = attachedEffect->Get_EffectComponent();

                if (ImGui::TreeNode(rowLabel.c_str()))
                {
                    ImGui::Text("Asset : %s", effectName.c_str());
                    ImGui::Text("Playing : %s", (effectCom && effectCom->Is_Playing()) ? "true" : "false");
                    ImGui::Text("Loop Override : %s", attachedEffect->Get_LoopOverride() ? "true" : "false");
                    ImGui::Text("Tracking Bone : %s", attachedEffect->Is_TrackingBone() ? "true" : "false");

                    const string boneName = attachedEffect->Get_TargetBoneName().empty()
                        ? "(None)"
                        : attachedEffect->Get_TargetBoneName();
                    ImGui::Text("Bone : %s", boneName.c_str());

                    const Vec3 worldPos = attachedEffect->Get_Transform()->Get_WorldPosition();
                    ImGui::Text("World Pos : %.2f, %.2f, %.2f", worldPos.x, worldPos.y, worldPos.z);

                    if (effectCom)
                    {
                        ImGui::Text("LifeSpan : %.3f", effectCom->Get_LifeSpan());
                        ImGui::Text("Layer Count : %zu", effectCom->Get_ActiveLayers().size());
                    }

                    ImGui::TreePop();
                }
            }
        }

        ImGui::TreePop();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

shared_ptr<Inspector> Inspector::Create()
{
    return make_shared<Inspector>();
}
