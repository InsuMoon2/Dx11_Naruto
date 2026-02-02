#ifndef Engine_Macro_h__
#define Engine_Macro_h__

namespace Engine
{
#define ETOI(ENUM) static_cast<unsigned int>(ENUM)

#ifndef			MSG_BOX
#define			MSG_BOX(_message)			MessageBox(nullptr, TEXT(_message), L"System Message", MB_OK)
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

    // LOG
#define LOG_INFO(...)    spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)    spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)   spdlog::error(__VA_ARGS__)

// ==================================================
// NULL/FAILED 체크 매크로
// ==================================================
#define CHECK_NULL(_ptr) \
	        { if(_ptr == nullptr) { return; } }

#define CHECK_NULL_RETURN(_ptr, _return) \
	        { if(_ptr == nullptr) { return _return; } }

#define CHECK_FAILED(_hr) \
	        if(FAILED(_hr)) { MSG_BOX("Failed"); return E_FAIL; }

#define CHECK_FAILED_RETURN(_hr, _return) \
	        if(FAILED(_hr)) { MSG_BOX("Failed"); return _return; }


/* ------------------------------------------ */
/*            weak_ptr Lock 매크로             */
/* ------------------------------------------ */

// weak_ptr가 가리키는 객체를 안전하게 참조(Reference Count 처리)하기 위해 
// lock()을 통해 shared_ptr로 승격 후 사용
#define LOCK_WP(weak_ptr, var_name, ...)                                            \
        auto var_name = (weak_ptr).lock();                                          \
        if (!var_name)                                                              \
        {                                                                           \
            LOG_ERROR(#weak_ptr " Lock Failed");                                    \
            return __VA_ARGS__;                                                     \
        }

// ==================================================
// 싱글톤 매크로
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


// =======================
// 클래스 이름 세팅
// =======================
#define GENERATE_BODY(ClassName)                                         \
public:                                                                  \
    static const wchar_t* StaticClassName() { return L#ClassName; }      \
private:                                                                 \
                                                                         \
    bool _name_setter_ = [this](){                                       \
        if (this->Get_Name().empty())                                    \
            this->Set_Name(StaticClassName());                           \
        return true;                                                     \
    }();
}

#endif // Engine_Macro_h__
