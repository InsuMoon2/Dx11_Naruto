#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class UIObject;

class ENGINE_DLL UI_Manager : public Base
{
public:
    explicit                 UI_Manager();
    virtual                 ~UI_Manager() = default;

public:
    HRESULT                 Initialize();
    void                    Priority_Update(float timeDelta);
    void                    Update(float timeDelta);
    void                    Late_Update(float timeDelta);

public:
    HRESULT                 Add_UI(EUILayer layer, Shared<UIObject> uiObject);
    Shared<UIObject>        Find_UI(const wstring& name);

    void                    Show_UI(const wstring& name);
    void                    Hide_UI(const wstring& name);
    void                    Toggle_UI(const wstring& name);

    void                    Hide_All_Layer(EUILayer layer);
    void                    Hide_All_UI();
    void                    Clear_UI();

    void                    Notify_Viewport_Resize(float widht, float height);
    bool                    Is_InputBlocked() const;

private:
    using UIList = list<Shared<UIObject>>;
    UIList  _uiLayers[ETOI(EUILayer::END)];

    umap<wstring, Shared<UIObject>> _uiMap;

public:
    static Unique<UI_Manager> Create();
    virtual void Free() override;

};

NS_END
