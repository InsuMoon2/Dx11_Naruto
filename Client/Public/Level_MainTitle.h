#pragma once

#include "Level.h"

NS_BEGIN(Engine)
class UI_Text;
NS_END

NS_BEGIN(Client)

enum class ETitleState { PressSpace, SelectMenu };

class Background;
class UI_MainTitleMenuButton;

enum class EMainTitle
{
    BG_0, BG_1,
    Logo,
    PressText0, PressText1, PressText2,
    TitleMenuBtn,
};

class Level_MainTitle final : public Level
{
public:
    explicit Level_MainTitle(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_MainTitle();

public:
    virtual HRESULT Initialize();
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT         Ready_Layer_UI();

    void            Update_PressPhase();
    void            Update_SelectPhase();
    void            Apply_TitlePhase();
    void            Apply_MenuSelection();
    void            Execute_SelectedMenu();

private:
    ETitleState _titleState = ETitleState::PressSpace;

    Shared<Background> _pressText;
    array<Shared<UI_MainTitleMenuButton>, 3> _menuText {};
    int32 _selectedIndex = 0;

public:
    static shared_ptr<Level_MainTitle> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    virtual void Free() override;

};

NS_END
