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
#include "AnimationStateComponent.h"
#include "EffectComponent.h"
#include "Model.h"

// 런타임 이펙트 레이어 타입 이름을 인스펙터에 짧게 표시할 때 호출한다.
static string Get_EffectLayerKindLabel(Engine::EEffectLayerKind kind)
{
    const auto enumName = magic_enum::enum_name(kind);
    if (!enumName.empty())
        return string(enumName);

    return "Unknown";
}

// 메인 Inspector 창에서 선택 오브젝트를 그릴 때 무거운 컴포넌트를 제외할지 판별한다.
// Model/AnimationState는 몬스터처럼 애니메이션이 많은 오브젝트를 선택했을 때
// 매 프레임 큰 데이터를 다시 만들면서 프레임 드랍을 유발하므로 메인 Inspector에서는 숨긴다.
static bool Should_SkipHeavyComponentInMainInspector(Shared<Component> component)
{
    if (!component)
        return true;

    if (dynamic_pointer_cast<Model>(component))
        return true;

    if (dynamic_pointer_cast<Client::AnimationStateComponent>(component))
        return true;

    return false;
}

// ImGui에서 Vec3 값을 한 줄로 편집하고 변경 여부를 바로 받을 때 호출한다.
static bool Draw_Vec3Control(const char* label, Vec3& value, float speed = 0.1f)
{
    float editValue[3] = { value.x, value.y, value.z };
    if (!ImGui::DragFloat3(label, editValue, speed))
        return false;

    value = Vec3(editValue[0], editValue[1], editValue[2]);
    return true;
}

// ImGui에서 Color를 rgb 위주로 편집하고 alpha는 유지할 때 호출한다.
static bool Draw_Color3Control(const char* label, Color& value)
{
    float editValue[3] = { value.x, value.y, value.z };
    if (!ImGui::ColorEdit3(label, editValue))
        return false;

    value.x = editValue[0];
    value.y = editValue[1];
    value.z = editValue[2];
    return true;
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
        if (_isPrimaryShadowLightTarget)
        {
            Draw_PrimaryShadowLightInspector();
        }
        else if (_targetObject != nullptr)
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

void Inspector::Set_PrimaryShadowLightTarget(bool enabled)
{
    _isPrimaryShadowLightTarget = enabled;

    if (enabled)
        _targetObject = nullptr;
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
        if (!comp)
            continue;

        if (Should_SkipHeavyComponentInMainInspector(comp))
            continue;

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

void Inspector::Draw_PrimaryShadowLightInspector()
{
    ImGui::Text("Name: Primary Shadow Light");
    ImGui::Separator();

    bool overrideEnabled = GAME->Is_EditorPrimaryShadowLightOverrideEnabled();
    if (ImGui::Checkbox("Editor Override", &overrideEnabled))
    {
        GAME->Set_EditorPrimaryShadowLightOverrideEnabled(overrideEnabled);
    }

    const FLightDesc* sourceDesc = overrideEnabled
        ? GAME->Get_EditorPrimaryShadowLightOverrideDesc()
        : GAME->Get_PrimaryShadowLightDesc();

    if (!sourceDesc)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("Primary shadow light is not available.");
        return;
    }

    FLightDesc editableDesc = *sourceDesc; // 인스펙터에서 수정 중인 임시 사본이다.
    bool changed = false;

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.f, 0.85f, 0.35f, 1.f), ICON_FA_SUN " Directional Light");

    Vec3 lightDirection(editableDesc.direction.x, editableDesc.direction.y, editableDesc.direction.z); // 음영 계산에 쓰는 directional light 방향이다.
    changed |= Draw_Vec3Control("Direction", lightDirection, 0.01f);
    editableDesc.direction = Vec4(lightDirection.x, lightDirection.y, lightDirection.z, editableDesc.direction.w);

    changed |= Draw_Color3Control("Diffuse", editableDesc.diffuse);
    changed |= Draw_Color3Control("Ambient", editableDesc.ambient);
    changed |= Draw_Color3Control("Specular", editableDesc.specular);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.f, 1.f), ICON_FA_CAMERA " Shadow Camera");

    changed |= ImGui::Checkbox("Cast Shadow", &editableDesc.castShadow);
    changed |= ImGui::Checkbox("Use Shadow Camera", &editableDesc.useShadowCamera);

    if (editableDesc.useShadowCamera)
        ImGui::TextDisabled("Perspective mode: Eye/Target/FOV/Aspect/Near/Far are used.");
    else
        ImGui::TextDisabled("Orthographic mode: Center/Ortho Width/Height drive the shadow frustum.");

    ImGui::BeginDisabled(editableDesc.useShadowCamera);
    changed |= Draw_Vec3Control("Shadow Center", editableDesc.shadowCenter, 0.1f);
    changed |= ImGui::DragFloat("Shadow Ortho Width", &editableDesc.shadowOrthoWidth, 1.f, 1.f, 10000.f);
    changed |= ImGui::DragFloat("Shadow Ortho Height", &editableDesc.shadowOrthoHeight, 1.f, 1.f, 10000.f);
    ImGui::EndDisabled();

    if (editableDesc.useShadowCamera)
    {
        // Eye/Target을 같은 delta로 같이 옮겨서 shadow camera가 찍는 월드 위치를 직접 이동한다.
        Vec3 shadowRigPosition = editableDesc.shadowTarget;
        if (Draw_Vec3Control("Shadow Rig Position", shadowRigPosition, 0.5f))
        {
            // Shadow Rig Position은 target 기준 위치이며, delta를 eye/target/center에 함께 적용한다.
            const Vec3 shadowRigDelta = shadowRigPosition - editableDesc.shadowTarget;
            editableDesc.shadowEye += shadowRigDelta;
            editableDesc.shadowTarget += shadowRigDelta;
            editableDesc.shadowCenter += shadowRigDelta;
            changed = true;
        }

        ImGui::TextDisabled("Move Rig Position first. Eye changes angle/distance, Target changes look direction.");

        if (GAME->Current_Level() == ETOI(ELevelType::GamePlay))
        {
            // Gameplay의 Tutorial arena는 원점 주변이므로 shadow rig를 Area 내부 기본 위치로 되돌릴 때 사용한다.
            if (ImGui::Button("Focus Gameplay Area"))
            {
                Vec3 lightDir(editableDesc.direction.x, editableDesc.direction.y, editableDesc.direction.z);
                if (lightDir.LengthSquared() > FLT_EPSILON)
                    lightDir.Normalize();
                else
                    lightDir = Vec3(-1.f, -1.f, -1.f);

                const Vec3 gameplayAreaTarget = Vec3(0.f, 3.f, 0.f);
                editableDesc.useShadowCamera = false;
                editableDesc.shadowTarget = gameplayAreaTarget;
                editableDesc.shadowCenter = gameplayAreaTarget;
                editableDesc.shadowEye = gameplayAreaTarget - lightDir * 95.f;
                editableDesc.shadowOrthoWidth = 80.f;
                editableDesc.shadowOrthoHeight = 45.f;
                editableDesc.shadowFovY = 2.2f;
                editableDesc.shadowFar = 220.f;
                editableDesc.shadowNear = 0.1f;
                editableDesc.shadowStrength = 1.f;
                editableDesc.shadowSoftness = 0.35f;
                editableDesc.shadowAspect = max(1.f, GAME->Get_ViewportWidth()) / max(1.f, GAME->Get_ViewportHeight());
                changed = true;
            }
        }
    }
    else if (GAME->Current_Level() == ETOI(ELevelType::GamePlay))
    {
        // Ortho mode에서도 Gameplay shadow focus를 원점 주변 arena로 즉시 되돌릴 때 사용한다.
        if (ImGui::Button("Focus Gameplay Area"))
        {
            Vec3 lightDir(editableDesc.direction.x, editableDesc.direction.y, editableDesc.direction.z);
            if (lightDir.LengthSquared() > FLT_EPSILON)
                lightDir.Normalize();
            else
                lightDir = Vec3(-1.f, -1.f, -1.f);

            const Vec3 gameplayAreaTarget = Vec3(0.f, 3.f, 0.f);
            editableDesc.shadowTarget = gameplayAreaTarget;
            editableDesc.shadowCenter = gameplayAreaTarget;
            editableDesc.shadowEye = gameplayAreaTarget - lightDir * 95.f;
            editableDesc.shadowOrthoWidth = 80.f;
            editableDesc.shadowOrthoHeight = 45.f;
            editableDesc.shadowFovY = 2.2f;
            editableDesc.shadowFar = 220.f;
            editableDesc.shadowNear = 0.1f;
            editableDesc.shadowStrength = 1.f;
            editableDesc.shadowSoftness = 0.35f;
            editableDesc.shadowAspect = max(1.f, GAME->Get_ViewportWidth()) / max(1.f, GAME->Get_ViewportHeight());
            changed = true;
        }
    }

    changed |= Draw_Vec3Control("Shadow Eye", editableDesc.shadowEye, 0.1f);
    changed |= Draw_Vec3Control("Shadow Target", editableDesc.shadowTarget, 0.1f);
    changed |= ImGui::DragFloat("Shadow FOV Y", &editableDesc.shadowFovY, 0.005f, XMConvertToRadians(5.f), XMConvertToRadians(179.f));
    changed |= ImGui::DragFloat("Shadow Aspect", &editableDesc.shadowAspect, 0.01f, 0.1f, 4.f);
    changed |= ImGui::DragFloat("Shadow Near", &editableDesc.shadowNear, 0.01f, 0.01f, 500.f);
    changed |= ImGui::DragFloat("Shadow Far", &editableDesc.shadowFar, 0.5f, 1.f, 10000.f);
    changed |= ImGui::DragFloat("Shadow Bias", &editableDesc.shadowBias, 0.00005f, 0.f, 0.05f, "%.6f");
    changed |= ImGui::DragFloat("Shadow Strength", &editableDesc.shadowStrength, 0.01f, 0.f, 1.f);
    changed |= ImGui::DragFloat("Shadow Softness", &editableDesc.shadowSoftness, 0.01f, 0.35f, 8.f);

    if (!overrideEnabled)
    {
        ImGui::Spacing();
        ImGui::TextDisabled("Enable Editor Override to edit values live.");
        return;
    }

    if (changed)
    {
        Vec3 normalizedDirection = lightDirection; // directional light 음영 계산에 바로 반영할 정규화된 방향 벡터다.
        if (normalizedDirection.LengthSquared() > FLT_EPSILON)
            normalizedDirection.Normalize();
        else
            normalizedDirection = Vec3(-1.f, -1.f, -1.f);

        editableDesc.direction = Vec4(normalizedDirection.x, normalizedDirection.y, normalizedDirection.z, 0.f);
        editableDesc.shadowNear = max(editableDesc.shadowNear, 0.01f);
        editableDesc.shadowFar = max(editableDesc.shadowFar, editableDesc.shadowNear + 1.f);
        editableDesc.shadowAspect = max(editableDesc.shadowAspect, 0.1f);
        editableDesc.shadowFovY = clamp(editableDesc.shadowFovY, XMConvertToRadians(5.f), XMConvertToRadians(179.f));
        editableDesc.shadowSoftness = max(editableDesc.shadowSoftness, 0.35f);

        GAME->Set_EditorPrimaryShadowLightOverrideDesc(editableDesc);
    }
}

shared_ptr<Inspector> Inspector::Create()
{
    return make_shared<Inspector>();
}
