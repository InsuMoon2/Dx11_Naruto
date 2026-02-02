#include "pch.h"
#include "Profiler_View.h"
#include "GameInstance.h"

Profiler_View::Profiler_View()
    : EditorWindow(TEXT("Profiler"))
{
}

Profiler_View::~Profiler_View()
{
}

void Profiler_View::Initialize()
{
    EditorWindow::Initialize();
}

void Profiler_View::Update(float timeDelta)
{
    if (timeDelta > 0.f)
        _currentFPS = 1.0f / timeDelta;
    else
        _currentFPS = 60.0f; 

    _fpsHistory.push_back(_currentFPS);

    if (_fpsHistory.size() > HISTORY_SIZE)
        _fpsHistory.erase(_fpsHistory.begin());

    // DrawCall 데이터
    _currentDrawCalls = GAME->Get_DrawCallCount();

    _drawCallHistory.push_back(_currentDrawCalls);

    if (_drawCallHistory.size() > HISTORY_SIZE)
        _drawCallHistory.erase(_drawCallHistory.begin());


}

void Profiler_View::OnGui()
{
    if (!IsActive())
        return;

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - 320, 40), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Performance Profiler", &_isActive, flags))
    {
        ImGui::End();
        return;
    }

    // === FPS 표시 ===
    ImGui::Text("FPS: %.1f", _currentFPS);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "(%.2f ms)", 1000.f / _currentFPS);

    // === DrawCall 표시 ===
    ImGui::Text("Draw Calls: %d", _currentDrawCalls);


    ImGui::Separator();
    // === FPS 그래프 ===
    if (ImPlot::BeginPlot("FPS", ImVec2(300, 100)))
    {
        ImPlot::SetupAxes("Time", "FPS");
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 120);
        ImPlot::PlotLine("FPS", _fpsHistory.data(), _fpsHistory.size());
        ImPlot::EndPlot();
    }

    // === DrawCall 그래프 ===
    if (ImPlot::BeginPlot("Draw Calls", ImVec2(300, 100)))
    {
        ImPlot::SetupAxes("Time", "Count");
        ImPlot::PlotLine("Draw Calls", _drawCallHistory.data(), _drawCallHistory.size());
        ImPlot::EndPlot();
    }
    ImGui::End();
}

void Profiler_View::RenderFPSGraph()
{

}

void Profiler_View::RenderDrawCallGraph()
{

}

shared_ptr<Profiler_View> Profiler_View::Create()
{
    return make_shared<Profiler_View>();
}
