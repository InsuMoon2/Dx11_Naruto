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

class Inspector : public EditorWindow
{
public:
    explicit Inspector();
    virtual ~Inspector();

public:
    void    Initialize() override;
    void    Update(float timeDelta) override;
    void    OnGui() override;

public:
    void    Set_Target(Shared<GameObject> target) { _targetObject = target; }

private:
    // 컴포넌트별 UI
    void Draw_Component(uint32 id, Shared<Component> component);

    void Draw_Transform(Shared<Transform> transform);
    void Draw_CombatStat();

private:
    Shared<GameObject> _targetObject;

public:
    static shared_ptr<Inspector> Create();

};

NS_END
