#include "pch.h"
#include "Input_Manager.h"

IMPLEMENT_SINGLETON(Input_Manager)

Input_Manager::Input_Manager()
{
}

Input_Manager::~Input_Manager()
{
}

void Input_Manager::Init(HWND hwnd)
{
    _hwnd = hwnd;
    _states.resize(KEY_TYPE_COUNT, KEY_STATE::NONE);

    // 화면 중앙 좌표 계산
    RECT rect;
    GetClientRect(_hwnd, &rect);
    _screenCenter.x = (rect.right - rect.left) / 2;
    _screenCenter.y = (rect.bottom - rect.top) / 2;
}

void Input_Manager::Update(float timeDleta)
{
    HWND hwnd = ::GetActiveWindow();
    if (_hwnd != hwnd)
    {
        for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
            _states[key] = KEY_STATE::NONE;

        // 창 비활성화 시 마우스 해제
        if (_mouseLocked)
            UnlockMouse();

        return;
    }

    // 키보드 처리
    BYTE asciiKeys[KEY_TYPE_COUNT] = {};
    if (::GetKeyboardState(asciiKeys) == false)
        return;

    for (uint32 key = 0; key < KEY_TYPE_COUNT; key++)
    {
        // 키가 눌려 있으면 true
        if (asciiKeys[key] & 0x80)
        {
            KEY_STATE& state = _states[key];

            // 이전 프레임에 키를 누른 상태라면 PRESS
            if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
                state = KEY_STATE::PRESS;
            else
                state = KEY_STATE::DOWN;
        }
        else
        {
            KEY_STATE& state = _states[key];

            // 이전 프레임에 키를 누른 상태라면 UP
            if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN)
                state = KEY_STATE::UP;
            else
                state = KEY_STATE::NONE;
        }
    }

    // 마우스
    if (_mouseLocked)
    {
        POINT currentPos;
        ::GetCursorPos(&currentPos);
        ::ScreenToClient(_hwnd, &currentPos);

        _mouseDelta.x = (float)(currentPos.x - _mousePos.x);
        _mouseDelta.y = (float)(currentPos.y - _mousePos.y);

        // 마우스를 화면 중앙으로 리셋
        POINT centerScreen = _screenCenter;
        ::ClientToScreen(_hwnd, &centerScreen);
        ::SetCursorPos(centerScreen.x, centerScreen.y);

        _mousePos = _screenCenter;
    }
    else
    {
        POINT currentPos;
        ::GetCursorPos(&currentPos);
        ::ScreenToClient(_hwnd, &currentPos);

        _mouseDelta.x = 0.f;
        _mouseDelta.y = 0.f;
        _mousePos = currentPos;
    }
}

void Input_Manager::LockMouse()
{
    if (_mouseLocked)
        return;

    _mouseLocked = true;

    ::ShowCursor(FALSE);

    POINT centerScreen = _screenCenter;
    ::ClientToScreen(_hwnd, &centerScreen);
    ::SetCursorPos(centerScreen.x, centerScreen.y);

    RECT rect;
    ::GetClientRect(_hwnd, &rect);
    ::ClientToScreen(_hwnd, (POINT*)&rect.left);
    ::ClientToScreen(_hwnd, (POINT*)&rect.right);
    ::ClipCursor(&rect);
}

void Input_Manager::UnlockMouse()
{
    if (!_mouseLocked)
        return;

    _mouseLocked = false;

    ::ShowCursor(TRUE);
    ::ClipCursor(nullptr);
}
