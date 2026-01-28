#include "pch.h"
#include "Content_Browser.h"

Content_Browser::Content_Browser()
    : EditorWindow(TEXT("Content Browser"))
{
}

Content_Browser::~Content_Browser()
{
}

void Content_Browser::Initialize()
{
    EditorWindow::Initialize();

}

void Content_Browser::Update()
{
    EditorWindow::Update();

}

void Content_Browser::OnGui()
{
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    ImGui::Begin("Content Browser", nullptr, flags);
    {

    }
    ImGui::End();
}

shared_ptr<Content_Browser> Content_Browser::Create()
{
    return make_shared<Content_Browser>();
}
