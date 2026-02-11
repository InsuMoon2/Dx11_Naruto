#pragma once

#include "Level.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(EditorApp)

class Level_Editor final : public Level
{
public:
    explicit Level_Editor(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Editor();

public:
    virtual HRESULT Initialize() override;

public:
    static shared_ptr<Level_Editor> Create(ComPtr<Device> device,
        ComPtr<DeviceContext> context);

    virtual void Free() override;
};

NS_END
