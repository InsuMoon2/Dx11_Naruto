#include "pch.h"
#include "UI_MainTitleText.h"

UI_MainTitleText::UI_MainTitleText(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_UI_MAIN_TITLE_TEXT);
}

UI_MainTitleText::UI_MainTitleText(const UI_MainTitleText& rhs)
    : UIObject(rhs)
{
}
