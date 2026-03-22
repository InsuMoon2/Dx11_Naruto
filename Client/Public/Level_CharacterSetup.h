#pragma once

#include "Level.h"

NS_BEGIN(Engine)
class UI_Text;
NS_END

NS_BEGIN(Client)

class Background;
class UI_TabButton;
class Player;

enum class ECharacterSetupTexture
{
    Background,
    Window,
    WinTitle,
    SelectDesc,
    TitleBG,
    Title_Symbol,
    SelectButton,
    TabButton,

    END
};

class Level_CharacterSetup final : public Level
{
public:
    explicit Level_CharacterSetup(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_CharacterSetup();

public:
    virtual HRESULT Initialize();
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT         Ready_Layer_UI();

private:
    array<Shared<UI_TabButton>, 6> _tabButton {};
    int32 _selectedIndex = 0;

public:
    static shared_ptr<Level_CharacterSetup> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    virtual void Free() override;

};

NS_END
