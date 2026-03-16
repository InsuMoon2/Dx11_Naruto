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
    void                    Set_UIPrototypeLevel(uint32 levelIndex) { _uiPrototypeLevel = levelIndex; }

    Shared<UIObject>        Add_UI(uint32 objID, EUILayer layer, void* arg);
    Shared<UIObject>        Clone_UI(uint32 objID, void* arg);
    HRESULT                 Register_UI(EUILayer layer, Shared<UIObject> uiObject);

    Shared<UIObject>        Find_UI(const wstring& name);

    void                    Show_UI(const wstring& name);
    void                    Hide_UI(const wstring& name);
    void                    Toggle_UI(const wstring& name);

    void                    Hide_All_Layer(EUILayer layer);
    void                    Hide_All_UI();
    void                    Clear_All_UI();
    void                    Clear_UI_ByLevel(uint32 levelIndex);

    void                    Notify_Viewport_Resize(float widht, float height);
    bool                    Is_InputBlocked() const;

    const   list<Shared<UIObject>>& Get_UILayer(EUILayer layer) const
    {
        return _uiLayers[ETOI(layer)];
    };

public: /* 애니메이션 재생용 */
    bool                    Play_UIAnimation(Shared<UIObject> target, const string& animationName);
    bool                    Play_UIAnimation(const wstring& targetName, const string& animationName);

    bool                    Pause_UIAnimation(Shared<UIObject> target, const string& animationName);
    bool                    Pause_UIAnimation(const wstring& targetName, const string& animationName);

    bool                    Stop_UIAnimation(Shared<UIObject> target, const string& animationName);
    bool                    Stop_UIAnimation(const wstring& targetName, const string& animationName);

private:
    uint32                  Get_UIPrototypeLevel() const { return _uiPrototypeLevel; }

    Shared<FUIAnimAsset>    Load_UIAnimationAsset(const string& animationName);
    wstring                 Resolve_UIAnimationPath(const string& animationName) const;
    FUIAnimPlaybackEntry*   Find_UIAnimationEntry(const Shared<UIObject>& target, const string& animationName);

    void                    Update_UIAnimations(float timeDelta);
    void                    Remove_ExpiredUIAnimations();
    void                    Clear_UIAnimations();
    void                    Clear_UIAnimations_ByLevel(uint32 levelIndex);

    static string           Extract_UIAnimationName(const wstring& fullPath);

private:
    using UIList = list<Shared<UIObject>>;
    UIList  _uiLayers[ETOI(EUILayer::END)];

    umap<wstring, Shared<UIObject>> _uiMap;
    uint32 _uiPrototypeLevel = static_cast<uint32>(-1);

    umap<string, Shared<FUIAnimAsset>> _uiAnimAssetCache;
    mutable umap<string, wstring>      _uiAnimPathCache;
    vector<FUIAnimPlaybackEntry>       _uiAnimEntries;

public:
    static Unique<UI_Manager> Create();
    virtual void Free() override;

};

NS_END
