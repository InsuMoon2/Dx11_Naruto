#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class GameObject;
class Layer;
NS_END

NS_BEGIN(Editor)

class Hierarchy : public EditorWindow
{
public:
    explicit Hierarchy();
    virtual ~Hierarchy();

public:
    void    Initialize() override;
    void    Update(float timeDelta) override;
    void    OnGui() override;

public:
    const vector<Shared<GameObject>>& Get_SelectedObject() const { return _selectedObjects; }
    const vector<Shared<GameObject>>& Get_LevelObjects() const { return _levelObjects; }
    const vector<Shared<GameObject>>& Get_CopyObjects() const { return _copiedObjects; }

    void Select_Object(Shared<GameObject> obj, bool isMultiSelect);

    void Update_SelectOutline(vector<Shared<GameObject>>& obj);

private:
    void Draw_SearchBar();
    void Draw_ObjectList();
    // Hierarchy 상단의 Lighting 섹션에서 primary shadow light 항목을 그릴 때 호출한다.
    void Draw_LightingNode();
    void Draw_ObjectNode(Shared<GameObject> gameObject, int index);

    void Handle_Shotcuts();

    bool Is_Selected(Shared<GameObject> obj);

private:
    vector<Shared<GameObject>>       _levelObjects;
    umap<wstring, shared_ptr<Layer>> _levelLayers;

    vector<Shared<GameObject>>      _selectedObjects;
    vector<Shared<GameObject>>      _copiedObjects;
    // Hierarchy에서 가짜 primary shadow light 항목이 선택되었는지 나타낸다.
    bool _isPrimaryShadowLightSelected = false;

    // 검색 필터
    string _currentSearchFilter;

public:
    static Shared<Hierarchy> Create();

};

NS_END
