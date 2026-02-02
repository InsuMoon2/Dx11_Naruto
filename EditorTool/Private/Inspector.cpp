#include "pch.h"
#include "Inspector.h"

Inspector::Inspector()
    : EditorWindow(TEXT("Inspector"))
{
}

Inspector::~Inspector()
{
}

void Inspector::Initialize()
{
    EditorWindow::Initialize();
}

void Inspector::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);
}

void Inspector::OnGui()
{
    ImGui::Begin("Inspector");
    {

    }
    ImGui::End();
}

void Inspector::Set_Target(shared_ptr<GameObject> target)
{

}

shared_ptr<Inspector> Inspector::Create()
{
    return make_shared<Inspector>();
}
