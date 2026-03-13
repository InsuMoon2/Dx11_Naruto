#pragma once

#include "Texture.h"

NS_BEGIN(Engine)

class GameObject;
class Component;

class Prototype_Manager : public Base
{
public:
    explicit Prototype_Manager();
    virtual ~Prototype_Manager();

public:
    HRESULT                     Initialize(uint32 numLevels);

    HRESULT                     Add_GameObject_Prototype(uint32 levelIndex, uint32 objID, shared_ptr<GameObject> prototype);
    shared_ptr<GameObject>      Clone_GameObject(uint32 levelIndex, uint32 objID, void* arg);

    HRESULT                     Add_Component_Prototype(uint32 levelIndex, uint32 componentID, shared_ptr<Component> prototype);
    shared_ptr<Component>       Clone_Component(uint32 levelIndex, uint32 componentID, void* arg);

    // 전역 세팅
    shared_ptr<Component>       Clone_Component(uint32 componentID, void* arg);

    HRESULT                     Clear_Prototype(uint32 levelIndex);

public:
    void                        Set_CurrentLevelIndex(uint32 index) { _currentLevelIndex = index; }

    vector<pair<uint32, wstring>> Get_RegisteredGameObjects();

    shared_ptr<Component>       Find_Component_Prototype(uint32 levelIndex, uint32 componentID);
private:
    shared_ptr<GameObject>      Find_GameObject_Prototype(uint32 levelIndex, uint32 objID);

private:
    // GameObject
    using GameObjectProto = umap<uint32, shared_ptr<GameObject>>;
    vector<GameObjectProto> _gameObjectPrototypes;

    // Component
    using ComponentProto = umap<uint32, shared_ptr<Component>>;
    vector<ComponentProto> _componentPrototypes;

    uint32 _numLevels = { };

    uint32 _staticLevelIndex = 0;
    uint32 _currentLevelIndex = 0;

public:
    static unique_ptr<Prototype_Manager> Create(uint32 numLevels);
    virtual void Free() override;

};


NS_END
