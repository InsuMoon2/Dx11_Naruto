#pragma once

#include "EditorWindow.h"

NS_BEGIN(Client)
class CombatStat;
NS_END

NS_BEGIN(Engine)
class GameObject;
class Component;
class Transform;
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
    Shared<GameObject> _targetObject;

public:
    static shared_ptr<Inspector> Create();
};

NS_END
