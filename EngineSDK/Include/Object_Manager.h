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


private:
    using LayerType = umap<wstring, shared_ptr<Layer>>;
    vector<LayerType> _layers;

public:
    static unique_ptr<Object_Manager> Create(uint32 numLevels);
    virtual void Free() override;

};

NS_END
