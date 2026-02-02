#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;

class Layer : public Base
{
public:
    explicit Layer();
    virtual ~Layer();

public:
    HRESULT Add_GameObject(shared_ptr<GameObject> gameObject);
    void    Priority_Update(float timeDelta);
    void    Update(float timeDelta);
    void    Late_Update(float timeDelta);

    const list<shared_ptr<GameObject>>& Get_GameObjects() const { return _gameObjects; }

private:
    list<shared_ptr<GameObject>> _gameObjects;

public:
    static shared_ptr<Layer> Create();
    void Free() override;
};

NS_END
