#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class GameObject;
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
    const vector<shared_ptr<GameObject>>& Get_SelectedObject() const { return _selectedObjects; }
    const vector<shared_ptr<GameObject>>& Get_LevelObjects() const { return _levelObjects; }
    const vector<shared_ptr<GameObject>>& Get_CopyObjects() const { return _copiedObjects; }

private:
    void Draw_SearchBar();
    void Draw_ObjectList();

    void Handle_Shotcuts();

    bool Is_Selected(shared_ptr<GameObject> obj);
    void Select_Object(shared_ptr<GameObject> obj, bool isMultiSelect);

private:
    vector<shared_ptr<GameObject>>  _levelObjects;

    vector<shared_ptr<GameObject>>  _selectedObjects;
    vector<shared_ptr<GameObject>>  _copiedObjects;

    // 검색 필터
    string _currentSearchFilter;

public:
    static shared_ptr<Hierarchy> Create();

};

NS_END
