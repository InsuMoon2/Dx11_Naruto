#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Layer;

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

    HRESULT Add_GameObject(
            uint32 protoLevelIndex, const wstring& protoTag,
            uint32 layerLevelIndex, const wstring& layerTag,
            void* arg);

private:
    shared_ptr<Layer> Find_Layer(uint32 levelIndex, const wstring& layerTag);

private:
    using LayerType = umap<wstring, shared_ptr<Layer>>;
    vector<LayerType> _layers;

    uint32  _numLevels = { };

public:
    static unique_ptr<Object_Manager> Create(uint32 numLevels);
    virtual void Free() override;

};

NS_END
