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

    #define GAME CGameInstance::GetInstance()
    #define GET_SINGLE(classname) classname::GetInstance()

	// ==================================================
	// NULL/FAILED 체크 매크로
	// ==================================================
	#define NULL_CHECK(_ptr) \
	        { if(_ptr == nullptr) { return; } }

	#define NULL_CHECK_RETURN(_ptr, _return) \
	        { if(_ptr == nullptr) { return _return; } }

	#define FAILED_CHECK(_hr) \
	        if(FAILED(_hr)) { MSG_BOX("Failed"); return E_FAIL; }

	#define FAILED_CHECK_RETURN(_hr, _return) \
	        if(FAILED(_hr)) { MSG_BOX("Failed"); return _return; }
	
	
	// ==================================================
	// 싱글톤 매크로
	// ==================================================
	#define NO_COPY(CLASSNAME)											 \
	        private:													 \
	        CLASSNAME(const CLASSNAME&) = delete;						 \
	        CLASSNAME& operator=(const CLASSNAME&) = delete;			 
																		 
	#define DECLARE_SINGLETON(CLASSNAME)								 \
	        NO_COPY(CLASSNAME)											 \
	        private:													 \
	        static std::shared_ptr<CLASSNAME> m_pInstance;				 \
	        public:														 \
	        static std::shared_ptr<CLASSNAME> GetInstance();			 \
	        static void DestroyInstance();

	#define IMPLEMENT_SINGLETON(CLASSNAME)								 \
	        std::shared_ptr<CLASSNAME> CLASSNAME::m_pInstance = nullptr; \
	        std::shared_ptr<CLASSNAME> CLASSNAME::GetInstance() {		 \
	            if(m_pInstance == nullptr) {							 \
	                m_pInstance = std::make_shared<CLASSNAME>();		 \
	            }														 \
	            return m_pInstance;										 \
	        }															 \
	        void CLASSNAME::DestroyInstance() {							 \
	            m_pInstance.reset();									 \
	        }
}

#endif // Engine_Macro_h__
