#pragma once

#include "Level.h"

NS_BEGIN(Engine)
class UI_Text;
NS_END

NS_BEGIN(Client)

class Background;
class UI_EquipmentTabButton;
class Player;

class Level_Equipment final : public Level
{
public:
    explicit Level_Equipment(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Equipment();

public:
    virtual HRESULT Initialize();
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT         Ready_Layer_UI();

private:
    //array<Shared<UI_MainTitleMenuButton>, 3> _menuText {};
    int32 _selectedIndex = 0;

public:
    static shared_ptr<Level_Equipment> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    virtual void Free() override;

};

NS_END
