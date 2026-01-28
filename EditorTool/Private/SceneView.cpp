#include "pch.h"
#include "SceneView.h"

SceneView::SceneView()
    : EditorWindow(TEXT("Scene"))
{
}

SceneView::~SceneView()
{
}

void SceneView::Initialize()
{
    EditorWindow::Initialize();

}

void SceneView::Update()
{
    EditorWindow::Update();

}

void SceneView::OnGui()
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin("Scene", nullptr, flags);
    {
        
    }
    ImGui::End();
}

shared_ptr<SceneView> SceneView::Create()
{
    return make_shared<SceneView>();
}
