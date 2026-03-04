#pragma once

#include "Base.h"

NS_BEGIN(Engine)
struct FEvent;

class Layer;
class GameObject;

class Object_Manager : public Base
{
public:
    explicit Object_Manager();
    virtual ~Object_Manager();

public:
    HRESULT Initialize(uint32 numLevels);
    void    Priority_Update(float timeDelta);
    void    Update(float timeDelta);
    void    Late_Update(float timeDelta);

    void    Clear_Layers(uint32 levelIndex);

public:
    HRESULT Add_GameObject(
            uint32 protoLevelIndex, uint32 objID,
            uint32 layerLevelIndex, const wstring& layerTag, void* arg);

    Shared<GameObject>      Clone_And_Add_GameObject(
            uint32 protoIndex, uint32 objID,
            uint32 levelIndex, const wstring& layerTag, void* arg);

    // 이미 생성된 포인터 등록
    HRESULT Add_GameObject(uint32 levelIndex, const wstring& layerTag, shared_ptr<GameObject> gameObject);


    void Delete_GameObject(uint32 levelIndex, shared_ptr<GameObject> gameObject);

    vector<shared_ptr<GameObject>> Get_GameObjects(uint32 levelIndex);

    const umap<wstring, shared_ptr<Layer>>& Get_Layers(uint32 levelIndex);

private:
    void Bind_Events();
    shared_ptr<Layer> Find_Layer(uint32 levelIndex, const wstring& layerTag);

    void OnCreateEvent(shared_ptr<FEvent> event);
    void OnDeleteEvent(shared_ptr<FEvent> event);

private:
    using LayerType = umap<wstring, shared_ptr<Layer>>;
    vector<LayerType> _layers;

    uint32  _numLevels = { };

public:
    static unique_ptr<Object_Manager> Create(uint32 numLevels);
    virtual void Free() override;

};

NS_END
