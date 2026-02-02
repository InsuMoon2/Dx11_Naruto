#pragma once

namespace Server
{
#define NS_BEGIN(NAMESPACE)    namespace NAMESPACE {
#define NS_END                 }

    // 유틸리티 매크로
#define CHECK_NULL(_ptr) \
        { if(_ptr == nullptr) { return; } }

#define CHECK_NULL_RETURN(_ptr, _return) \
        { if(_ptr == nullptr) { return _return; } }

    // weak_ptr Lock
#define LOCK_WP(weak_ptr, var_name, ...)                 \
        auto var_name = (weak_ptr).lock();                   \
        if (!var_name)                                       \
        {                                                    \
            cout << #weak_ptr " Lock Failed" << endl;        \
            return __VA_ARGS__;                              \
        }

    // 싱글톤 (서버용)
#define NO_COPY(CLASSNAME)                               \
        private:                                             \
        CLASSNAME(const CLASSNAME&) = delete;                \
        CLASSNAME& operator=(const CLASSNAME&) = delete;

#define DECLARE_SINGLETON(CLASSNAME)                     \
        NO_COPY(CLASSNAME)                                   \
        private:                                             \
        static std::shared_ptr<CLASSNAME> m_pInstance;       \
        public:                                              \
        static std::shared_ptr<CLASSNAME>& GetInstance();    \
        static void DestroyInstance();

#define IMPLEMENT_SINGLETON(CLASSNAME)                   \
        std::shared_ptr<CLASSNAME> CLASSNAME::m_pInstance = nullptr; \
        std::shared_ptr<CLASSNAME>& CLASSNAME::GetInstance() { \
            if(m_pInstance == nullptr) {                     \
                m_pInstance = std::make_shared<CLASSNAME>(); \
            }                                                \
            return m_pInstance;                              \
        }                                                    \
        void CLASSNAME::DestroyInstance() {                  \
            m_pInstance.reset();                             \
        }
}

