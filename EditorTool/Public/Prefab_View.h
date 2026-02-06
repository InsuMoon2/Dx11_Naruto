#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class GameObject;
NS_END

NS_BEGIN(Editor)

class Prefab_View : public EditorWindow
{
public:
    explicit Prefab_View();
    virtual ~Prefab_View();

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;

public:
    void Open_Prefab(const string& prefabName, const string& prefabPath);
    void Close_Prefab();

    bool Is_Open() const { return _isOpen; }
private:

    void Draw_Header();
    void Draw_ComponentList();
    void Draw_Buttons();

public:
    static shared_ptr<Prefab_View> Create();

private:
    bool _isOpen = false;

    string _prefabName;
    string _prefabPath;

    Shared<GameObject> _targetObject;
};

NS_END
