#include "pch.h"
#include "PlayerSession_Manager.h"

PlayerSession_Manager::~PlayerSession_Manager()
{
    Stop_AllSession();
}

void PlayerSession_Manager::Start_SinglePlayer()
{
    _editorMainWindow = GetActiveWindow();
    //ShowWindow(_editorMainWindow, SW_MINIMIZE);

    // 싱글 플레이어 아닐 때 예외처리
    if (!Launch_Client(0, 1))
    {
        MessageBox(nullptr, L"Failed to launch client!", L"Error", MB_OK | MB_ICONERROR);
        ShowWindow(_editorMainWindow, SW_RESTORE);
    }

}

void PlayerSession_Manager::Start_MultiPlayer(int32 playerCount)
{
    _editorMainWindow = GetActiveWindow();
    //ShowWindow(_editorMainWindow, SW_HIDE);

    // 서버 없을 때 예외처리
    if (!Launch_Server())
    {
        MessageBox(nullptr, L"Failed to launch GameServer.exe!", L"Error", MB_OK | MB_ICONERROR);
        ShowWindow(_editorMainWindow, SW_SHOW);
        return;
    }

    Sleep(2000);

    for (int32 i = 0; i < playerCount; i++)
    {
        if (!Launch_Client(i, playerCount))
        {
            MessageBox(nullptr, L"Failed to launch client!", L"Error", MB_OK | MB_ICONERROR);
            break;
        }

        Sleep(500);
    }
}

void PlayerSession_Manager::Stop_AllSession()
{
    for (auto& pi : _activeProcesses)
    {
        if (pi.hProcess)
        {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    _activeProcesses.clear();

    if (_editorMainWindow)
    {
        ShowWindow(_editorMainWindow, SW_SHOW);
        SetForegroundWindow(_editorMainWindow);
    }
}

bool PlayerSession_Manager::Launch_Server()
{
    fs::path currentPath = fs::current_path();
    fs::path serverPath = currentPath / L"../../Server/GameServer/Bin/GameServer.exe";

    if (!fs::exists(serverPath))
        return false;

    STARTUPINFO startInfo = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION processInfo = {};

    BOOL result = CreateProcess(
        serverPath.wstring().c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_CONSOLE,
        nullptr,
        serverPath.parent_path().wstring().c_str(),
        &startInfo,
        &processInfo
    );

    if (!result)
        return false;

    _activeProcesses.emplace_back(processInfo);

    return true;
}

bool PlayerSession_Manager::Launch_Client(int32 playerIndex, int32 totalPlayers)
{
    fs::path currentPath = fs::current_path();
    fs::path clientPath = currentPath / L"../../Client/Bin/Client.exe";

    if (!fs::exists(clientPath))
        return false;

    int32 posX, posY, width, height;
    Calculate_WindowLayout(playerIndex, totalPlayers, posX, posY, width, height);

    wstring args = L"--player" + to_wstring(playerIndex) +
        L" --x" + to_wstring(posX) +
        L" --y" + to_wstring(posY) +
        L" --width" + to_wstring(width) +
        L" --height" + to_wstring(height) +
        L" --no-editor";

    fs::path canonicalPath = fs::canonical(clientPath);

    wstring fullCmd = L"\"" + canonicalPath.wstring() + L"\" " + args;
    vector<wchar_t> cmdBuf(fullCmd.begin(), fullCmd.end());
    cmdBuf.push_back(L'\0');

    STARTUPINFO startInfo = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION processInfo = {};

    BOOL result = CreateProcess(
        nullptr,
        cmdBuf.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        canonicalPath.parent_path().wstring().c_str(),
        &startInfo,
        &processInfo
    );

    if (!result)
        return false;
    _activeProcesses.emplace_back(processInfo);

    return true;
}

void PlayerSession_Manager::Calculate_WindowLayout(int32 playerIndex, int32 totalPlayers, int32& outX, int32& outY,
    int32& outWidth, int32& outHeight)
{
    int32 screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int32 screenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (totalPlayers == 1)
    {
        outWidth = 1280;
        outHeight = 720;
        outX = (screenWidth - outWidth) / 2;
        outY = (screenHeight - outHeight) / 2;
    }
    else if (totalPlayers == 2)
    {
        outWidth = screenWidth / 2 - 40;
        outHeight = screenHeight - 150;
        outX = playerIndex * (screenWidth / 2) + 10;
        outY = 50;
    }
    else if (totalPlayers <= 4)
    {
        outWidth = screenWidth / 2 - 20;
        outHeight = screenHeight / 2 - 50;
        int32 col = playerIndex % 2;
        int32 row = playerIndex / 2;
        outX = col * (screenWidth / 2) + 10;
        outY = row * (screenHeight / 2) + 30;
    }
    else
    {
        int32 cols = static_cast<int32>(ceil(sqrt(static_cast<float>(totalPlayers))));
        int32 rows = static_cast<int32>(ceil(static_cast<float>(totalPlayers) / cols));
        outWidth = screenWidth / cols - 20;
        outHeight = screenHeight / rows - 50;
        int32 col = playerIndex % cols;
        int32 row = playerIndex / cols;
        outX = col * (screenWidth / cols) + 10;
        outY = row * (screenHeight / rows) + 30;
    }
}

unique_ptr<PlayerSession_Manager> PlayerSession_Manager::Create()
{
    return make_unique<PlayerSession_Manager>();
}
