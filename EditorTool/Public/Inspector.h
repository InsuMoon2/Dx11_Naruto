#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class GameObject;
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
    void    Set_Target(shared_ptr<GameObject> target);

public:
    static shared_ptr<Inspector> Create();

};

NS_END
