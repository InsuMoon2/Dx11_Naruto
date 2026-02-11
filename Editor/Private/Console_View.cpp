#include "pch.h"
#include "Console_View.h"

Console_View::Console_View()
    : EditorWindow(TEXT("Console"))
{
}

Console_View::~Console_View()
{
}

void Console_View::Initialize()
{
    EditorWindow::Initialize();
    Editor_Logger::Initialize();
}

void Console_View::Update(float timeDelta)
{
    EditorWindow::Update(timeDelta);
}

void Console_View::OnGui()
{
    string str = Utils::ToString(Get_Name());

    ImGui::Begin(str.c_str(), &_isActive);
    {
        auto sink = Editor_Logger::GetSink();

        if (ImGui::Button("Clear"))
            sink->ClearLogs();

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &_autoScroll);
        ImGui::Separator();

        ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        {
            const auto& logs = sink->GetLogs();

            for (const auto& log : logs)
            {
                ImVec4 color = ImVec4(1, 1, 1, 1); // 기본 흰색으로 Info

                if (log.level == LogLevel::Warning)
                    color = ImVec4(1, 1, 0, 1);

                else if (log.level == LogLevel::Error)
                    color = ImVec4(1, 0, 0, 1);

                ImGui::TextColored(color, "%s", log.message.c_str());
            }

            // 자동 스크롤
            if (_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.f);
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

shared_ptr<Console_View> Console_View::Create()
{
    return make_shared<Console_View>();
}
