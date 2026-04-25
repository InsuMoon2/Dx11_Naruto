#pragma once

#include "EditorWindow.h"

NS_BEGIN(Client)
class CombatStat;
NS_END

NS_BEGIN(Engine)
class GameObject;
class Component;
class Transform;
class ContainerObject;
NS_END

NS_BEGIN(Editor)

class Inspector : public EditorWindow {
public:
    explicit Inspector();
    virtual ~Inspector();

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;

public:
    void Set_Target(Shared<GameObject> target) { _targetObject = target; _isPrimaryShadowLightTarget = false; }
    // Hierarchy에서 primary shadow light를 선택했을 때 GameObject 대신 light 설정을 인스펙터에 노출할지 결정한다.
    void Set_PrimaryShadowLightTarget(bool enabled);
    // 현재 인스펙터가 GameObject 대신 primary shadow light를 그리고 있는지 확인한다.
    bool Is_PrimaryShadowLightTarget() const { return _isPrimaryShadowLightTarget; }

public:
    static void Draw_Component(uint32 id, Shared<Component> component);
    static void Draw_Components(Shared<GameObject> target);

private:
    static void Draw_RuntimeEffectDebug(Shared<GameObject> target);
    // 에디터에서 primary shadow light와 shadow camera 값을 직접 조절할 때 호출한다.
    void Draw_PrimaryShadowLightInspector();
    void Draw_PartObjects(Shared<ContainerObject> container);
    void Draw_SelectedPartObject(Shared<ContainerObject> container);

private:
    Shared<GameObject> _targetObject;
    // Hierarchy에서 가짜 Lighting 항목을 선택했을 때 Inspector가 light 전용 UI를 그릴지 나타낸다.
    bool _isPrimaryShadowLightTarget = false;
    int32 _selectedPartSlot = -1;

public:
    static shared_ptr<Inspector> Create();
};

NS_END
