#include "pch.h"
#include "Logger.h"

Logger::Logger()
    : EditorWindow(TEXT("Log"))
{
}

Logger::~Logger()
{
}

void Logger::Initialize()
{
    EditorWindow::Initialize();
}

void Logger::Update()
{
    EditorWindow::Update();
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
