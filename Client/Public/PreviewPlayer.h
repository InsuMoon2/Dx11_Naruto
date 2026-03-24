#pragma once

#include "Player.h"

NS_BEGIN(Client)

class PreviewPlayer final : public Player
{
    GENERATED_BODY(PreviewPlayer)

public:
    explicit PreviewPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit PreviewPlayer(const PreviewPlayer& rhs);
    virtual ~PreviewPlayer();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

protected:
    HRESULT Bind_ShaderResources() override;
    HRESULT Bind_Lights() override;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
