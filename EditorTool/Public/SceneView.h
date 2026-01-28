#pragma once

#include "EditorWindow.h"

NS_BEGIN(Editor)

class SceneView : public EditorWindow
{
public:
    explicit SceneView();
    virtual ~SceneView();

public:
    void    Initialize() override;
    void    Update() override;
    void    OnGui() override;

public:
    static shared_ptr<SceneView> Create();
   
};

NS_END
