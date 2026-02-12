#pragma once

NS_BEGIN(Editor)

class ImGui_Manager
{
public:
    ImGui_Manager() = default;
    ~ImGui_Manager();

public:
    HRESULT Initialize(HWND hWnd, ComPtr<Device> device, ComPtr<DeviceContext> context);
    void    Update(float timeDelta);
    void    Render();
    void    Free();

private:
    void    ImGuiStyleSetting();

public:
    static unique_ptr<ImGui_Manager> Create(HWND hWnd, ComPtr<Device> device, ComPtr<DeviceContext> context);

};

NS_END
