#pragma once

#include "Base.h"

NS_BEGIN(Engine)

enum class KEY_TYPE
{
    UP      = VK_UP,
    DOWN    = VK_DOWN,
    LEFT    = VK_LEFT,
    RIGHT   = VK_RIGHT,
    SHIFT   = VK_SHIFT,
    SPACE   = VK_SPACE,
    DEL     = VK_DELETE,
    CTRL    = VK_CONTROL,
    ENTER   = VK_RETURN,
    ESCAPE  = VK_ESCAPE,
    BACK    = VK_BACK,

    TAB     = VK_TAB,

    F1      = VK_F1,
    F2      = VK_F2,
    F3      = VK_F3,
    F4      = VK_F4,
    F5      = VK_F5,


    F8      = VK_F8,
    F9      = VK_F9,
    F11     = VK_F11,

    W       = 'W',
    A       = 'A',
    S       = 'S',
    D       = 'D',

    Q       = 'Q',
    E       = 'E',
    Z       = 'Z',
    C       = 'C',
    V       = 'V',
    R       = 'R',

    KEY_1   = '1',
    KEY_2   = '2',
    KEY_3   = '3',
    KEY_4   = '4',

    LBUTTON = VK_LBUTTON,
    RBUTTON = VK_RBUTTON,

    LCTRL   = VK_LCONTROL,

};

enum class KEY_STATE
{
    NONE,
    PRESS,
    DOWN,
    UP,
    END
};

enum
{
    KEY_TYPE_COUNT = static_cast<int32>(UINT8_MAX + 1),
    KEY_STATE_COUNT = static_cast<int32>(KEY_STATE::END),
};

class ENGINE_DLL Input_Manager : public Base
{
    DECLARE_SINGLETON(Input_Manager);

public:
    explicit Input_Manager();
    virtual ~Input_Manager();

public:
    void Init(HWND hwnd);
    void Update(float timeDelta);

    // 누르고 있을 때
    bool KeyPress(KEY_TYPE key) { return GetState(key) == KEY_STATE::PRESS; }
    // 맨 처음 눌렀을 때
    bool KeyDown(KEY_TYPE key) { return GetState(key) == KEY_STATE::DOWN; }
    // 맨 처음 눌렀다 뗐을 때
    bool KeyUp(KEY_TYPE key) { return GetState(key) == KEY_STATE::UP; }

    const POINT& GetMousePos() { return _mousePos; }

    // 마우스 캡처
    void LockMouse();
    void UnlockMouse();
    bool IsMouseLocked() const           { return _mouseLocked; }

    Vec2  GetMouseDelta() const          { return _mouseDelta; }
    float GetMouseWheel() const          { return _mouseWheelDelta; }

    void  Set_MouseWheel(float delta)    { _mouseWheelDelta = delta; }

    void  Set_InputBlocked(bool blocked)  { _inputBlocked = blocked; }
    bool  Is_InputBlocked() const         { return _inputBlocked; }

private:
    inline KEY_STATE GetState(KEY_TYPE key) { return _states[static_cast<uint8>(key)]; }

private:
    HWND _hwnd;
    vector<KEY_STATE> _states;
    POINT _mousePos = {};

    // 마우스 캡처
    bool    _mouseLocked = false;
    POINT   _screenCenter = { };
    Vec2    _mouseDelta = { };

    float   _mouseWheelDelta = 0.f;

    bool    _inputBlocked = false; // ImGui에서 키를 쓰고있으면 같이 안먹게

};

NS_END
