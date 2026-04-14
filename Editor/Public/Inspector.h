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
    void Set_Target(Shared<GameObject> target) { _targetObject = target; }

public:
    static void Draw_Component(uint32 id, Shared<Component> component);
    static void Draw_Components(Shared<GameObject> target);

private:
    void Draw_PartObjects(Shared<Engine::ContainerObject> container);
    void Draw_SelectedPartObject(Shared<Engine::ContainerObject> container);

private:
    Shared<GameObject> _targetObject;
    int32 _selectedPartSlot = -1;

public:
    static shared_ptr<Inspector> Create();
};

NS_END
