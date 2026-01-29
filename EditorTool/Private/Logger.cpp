#include "pch.h"
#include "Logger.h"

Logger::Logger()
    : EditorWindow(TEXT("Console"))
{
}

Logger::~Logger()
{
}

void Logger::Initialize()
{
    EditorWindow::Initialize();
}

void Logger::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);
}

void Logger::OnGui()
{
    ImGui::Begin("Console");
    {

    }
    ImGui::End();
}

shared_ptr<Logger> Logger::Create()
{
    return make_shared<Logger>();
}
