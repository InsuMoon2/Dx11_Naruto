#pragma once

namespace Engine
{
#define ETOI(ENUM) static_cast<unsigned int>(ENUM)

#ifndef			MSG_BOX
#define			MSG_BOX(_message)			MessageBox(nullptr, TEXT(_message), L"System Message", MB_OK)
#define			MSG_BOX_S(_message)			MessageBox(nullptr, _message, L"System Message", MB_OK)
#endif

#define			NS_BEGIN(NAMESPACE)		namespace NAMESPACE {
#define			NS_END						}

#define			USING(NAMESPACE)	    using namespace NAMESPACE;

#ifdef	ENGINE_EXPORTS
#define ENGINE_DLL		_declspec(dllexport)
#else
#define ENGINE_DLL		_declspec(dllimport)
#endif

#define GET_SINGLE(classname) classname::GetInstance()

#define GAME    GET_SINGLE(GameInstance)
#define INPUT	GET_SINGLE(Input_Manager)
#define EVENT	GET_SINGLE(Event_Manager)

// ==================================================
//              LOG 매크로
// ==================================================
#define LOG_INFO(...)    spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)    spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)   spdlog::error(__VA_ARGS__)

// ==================================================
//              NULL/FAILED 체크 매크로
// ==================================================
#define CHECK_NULL(_ptr, ...)                \
    if ((_ptr) == nullptr) {                 \
        LOG_ERROR("NULL");        \
        return __VA_ARGS__;                  \
    }

#define CHECK_FAILED(_hr, ...)               \
    if (FAILED(_hr)) {                       \
        LOG_ERROR("FAILED");       \
        return __VA_ARGS__;                  \
    }

// ==================================================
//              weak_ptr 매크로
// ==================================================

#define LOCK_WP(weak_ptr, var_name, ...)                                            \
        auto var_name = (weak_ptr).lock();                                          \
        if (!var_name)                                                              \
        {                                                                           \
            LOG_ERROR(#weak_ptr " Lock Failed");                                    \
            return __VA_ARGS__;                                                     \
        }

// ==================================================
//              싱글톤 매크로
// ==================================================
#define NO_COPY(CLASSNAME)											     \
	        private:													 \
	        CLASSNAME(const CLASSNAME&) = delete;						 \
	        CLASSNAME& operator=(const CLASSNAME&) = delete;			 

#define DECLARE_SINGLETON(CLASSNAME)								     \
	        NO_COPY(CLASSNAME)											 \
	        private:													 \
	        static std::shared_ptr<CLASSNAME> m_pInstance;				 \
	        public:														 \
	        static std::shared_ptr<CLASSNAME>& GetInstance();			 \
	        static void DestroyInstance();

#define IMPLEMENT_SINGLETON(CLASSNAME)								     \
	        std::shared_ptr<CLASSNAME> CLASSNAME::m_pInstance = nullptr; \
	        std::shared_ptr<CLASSNAME>& CLASSNAME::GetInstance() {		 \
	            if(m_pInstance == nullptr) {							 \
	                m_pInstance = std::make_shared<CLASSNAME>();		 \
	            }														 \
	            return m_pInstance;										 \
	        }															 \
	        void CLASSNAME::DestroyInstance() {							 \
	            m_pInstance.reset();									 \
	        }


// ==================================================
//              클래스 이름 세팅
// ==================================================
#define GENERATED_BODY(ClassName)                                         \
public:                                                                  \
    static const wchar_t* StaticClassName() { return L#ClassName; }      \
private:                                                                 \
                                                                         \
    bool _name_setter_ = [this](){                                       \
            this->Set_Name(StaticClassName());                           \
        return true;                                                     \
    }();

// ==================================================
//              컴포넌트 ID 세팅
// ==================================================

#define GENERATED_COMPONENT(ClassName, ProtoID)  \
    GENERATED_BODY(ClassName)                    \
                                                 \
public:                                          \
    /* Protobuf ID 반환 */                        \
    static uint32   StaticTypeID() { return static_cast<uint32>(ProtoID); } \
    virtual uint32  Get_ComponentID() const override { return StaticTypeID(); }



}
