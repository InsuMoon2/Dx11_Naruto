#include "pch.h"
#include "Inspector.h"

Inspector::Inspector()
    : EditorWindow(TEXT("Log"))
{
}

Inspector::~Inspector()
{
}

void Inspector::Initialize()
{
    EditorWindow::Initialize();
}

void Inspector::Update()
{
    EditorWindow::Update();
}

void Inspector::OnGui()
{
    ImGui::Begin("Inspector");
    {

    }
    ImGui::End();
}

shared_ptr<Inspector> Inspector::Create()
{
    return make_shared<Inspector>();
}
