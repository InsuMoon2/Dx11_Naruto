#pragma once
#include "UIObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL HUD : public UIObject
{
    GENERATED_BODY(HUD)

public:
    explicit HUD(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit HUD(const HUD& rhs);
    virtual ~HUD() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    void    Set_Visibility(bool active) override;

public:
    template<typename T>
    Shared<T> Create_Child(EUILayer uiLayer, void* arg);

    void Register_Child(Shared<UIObject> child);

    // HUD는 기본적으로 마우스 포커스 X
    virtual bool Is_Focusable() const { return false; }

protected:
    vector<Shared<UIObject>> _children;

public:
    virtual void Free() override;

};

template <typename T>
Shared<T> HUD::Create_Child(EUILayer uiLayer, void* arg)
{

    Shared<T> childUI = T::Create(_device, _context, arg);
    if (!childUI) return nullptr;

    childUI->Set_LevelIndex(this->_levelIndex);
    childUI->Set_UILayer(uiLayer);

    // 트랜스폼 부모 연결
    childUI->Get_Transform()->Set_Parent(this->Get_Transform());

    GAME->Add_UI_ToLayer(uiLayer, childUI);
    Register_Child(childUI);

    return childUI;
}


NS_END
