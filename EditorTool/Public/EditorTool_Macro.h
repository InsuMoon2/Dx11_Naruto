#pragma once

#define EDITOR EditorInstance::GetInstance()

// Notification
#define NOTIFY(...) EditorInstance::GetInstance()->Get_Notification()->Add_Notification(__VA_ARGS__)
#define NOTIFY_WARN(...) EditorInstance::GetInstance()->Get_Notification()->Add_Notification_With_Type(ENotifyType::Warning, __VA_ARGS__)

#define U8(str) (const char*)u8##str

// Insepctor
#define REGISTER_INSPECTOR(INSPECTOR, TYPE) \
    namespace { \
        static struct Inspector_Helper_##INSPECTOR { \
            Inspector_Helper_##INSPECTOR() { \
                Editor::Inspector_Factory::Register_Inspector(TYPE, make_shared<INSPECTOR>()); \
            } \
        } inspector_helper_##INSPECTOR; \
    }
