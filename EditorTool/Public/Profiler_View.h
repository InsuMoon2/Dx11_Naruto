#pragma once

#include "EditorWindow.h"

NS_BEGIN(Editor)

class Profiler_View : public EditorWindow
{
public:
    explicit Profiler_View();
    virtual ~Profiler_View();

public:
    virtual void Initialize() override;
    virtual void Update(float timeDelta) override;
    virtual void OnGui() override;

private:
    void RenderFPSGraph();
    void RenderDrawCallGraph();

private:
    // FPS 데이터
    static constexpr int HISTORY_SIZE = 120; // 2초 (60fps 기준)
    vector<float> _fpsHistory;
    float _currentFPS = 0.f;

    // DrawCall 데이터
    vector<int32> _drawCallHistory;
    int32 _currentDrawCalls = 0;

    // UI 상태
    bool _showPopup = false;


public:
    static shared_ptr<Profiler_View> Create();

};

NS_END
