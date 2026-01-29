#include "pch.h"
#include "HierarchyView.h"

HierarchyView::HierarchyView()
    : EditorWindow(TEXT("Hieararchy"))
{
}

HierarchyView::~HierarchyView()
{
}

void HierarchyView::Initialize()
{
    EditorWindow::Initialize();
}

void HierarchyView::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);
}

void HierarchyView::OnGui()
{
    ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoNavInputs);
    {

    }
    ImGui::End();
}

shared_ptr<HierarchyView> HierarchyView::Create()
{
    return make_shared<HierarchyView>();
}
