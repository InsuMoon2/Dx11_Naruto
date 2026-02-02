#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class GameObject;
NS_END

NS_BEGIN(Editor)

class HierarchyView : public EditorWindow
{
public:
    explicit HierarchyView();
    virtual ~HierarchyView();

public:
    void    Initialize() override;
    void    Update(float timeDelta) override;
    void    OnGui() override;

public:
    shared_ptr<GameObject> Get_SelectedObject() const { return _selectedObject; }
    const vector<shared_ptr<GameObject>>& Get_LevelObjects() const { return _levelObjects; }

private:
    void Draw_SearchBar();
    void Draw_ObjectList();

private:
    vector<shared_ptr<GameObject>>  _levelObjects;
    shared_ptr<GameObject>          _selectedObject;

    // 검색 필터
    string _currentSearchFilter;

public:
    static shared_ptr<HierarchyView> Create();

};

NS_END
