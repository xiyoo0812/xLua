#define LUA_LIB

#include "quanta.h"

#ifdef WIN32
#define getpid _getpid
#endif

namespace quanta {

    static quanta_app q_app;

    static const char* get_platform() {
    #if defined(__linux)
        return "linux";
    #elif defined(__APPLE__)
        return "apple";
    #elif defined(__NINTENDO__)
        return "nintendo";
    #elif defined(__ORBIS__) || defined(__PROSPERO__)
        return "playstation";
    #else
        return "windows";
    #endif
    }

    const char* quanta_app::get_env(const char* key) {
        auto it = m_environs.find(key);
        if (it != m_environs.end()) return it->second.c_str();
        return nullptr;
    }

    void quanta_app::set_env(const char* key, const char* value, int over) {
        if (over == 1 || m_environs.find(key) == m_environs.end()) {
            m_environs[key] = value;
        }
    }

    bool quanta_app::setup(lua_State* L, int argc, const char* argv[], std::string& serr) {
        m_lua = new kit_state(L);
        srand((unsigned)time(nullptr));
        //将启动参数转负责覆盖环境变量
        for (int i = 1; i < argc; ++i) {
            std::string argvi = argv[i];
            auto pos = argvi.find("=");
            if (pos != std::string::npos) {
                auto evalue = argvi.substr(pos + 1);
                auto ekey = std::format("QUANTA_{}", argvi.substr(2, pos - 2));
                std::transform(ekey.begin(), ekey.end(), ekey.begin(), [](auto c) { return std::toupper(c); });
                set_env(ekey.c_str(), evalue.c_str(), 1);
                continue;
            }
            if (i == 1){
                //加载LUA配置
                m_lua->set("platform", get_platform());
                m_lua->set_function("set_env", [&](const char* key, const char* value) { set_env(key, value, 1); });
                m_lua->set_function("set_path", [&](const char* field, const char* path) { m_lua->set_path(field, path); set_env(field, path, 1); });
                if (!m_lua->run_script(std::format("dofile('{}')", argv[1]), [&](std::string_view err) {
                    serr = std::format("load config err: {}", err);
                })) return false;
            }
        }
        return init(serr);
    }

    bool quanta_app::init(std::string& serr) {
        //初始化lua
        auto tid = std::this_thread::get_id();
        auto quanta = m_lua->get<lua_table>("quanta");
        quanta.set("pid", ::getpid());
        quanta.set("master", true);
        quanta.set("thread", "quanta");
        quanta.set("environs", m_environs);
        quanta.set("tid", *(uint32_t*)&tid);
        quanta.set("platform", get_platform());
        quanta.set_function("new_kitstate", [&]() { return new kit_state(); });
        quanta.set_function("getenv", [&](const char* key) { return get_env(key); });
        quanta.set_function("setenv", [&](const char* key, const char* value) { return set_env(key, value, 1); });

        auto sandbox = get_env("QUANTA_SANDBOX");
        if (sandbox) {
            if (!m_lua->run_script(std::format("require '{}'", sandbox), [&](std::string_view err) {
                serr = std::format("load sandbox err: {}", err);
            })) return false;
        }
        auto entry = get_env("QUANTA_ENTRY");
        if (!entry) {
            serr = std::format("load entry err: entry not found");
            return false;
        }
        if (!m_lua->run_script(std::format("require '{}'", entry), [&](std::string_view err) {
            serr = std::format("load entry err: {}", err);
        })) return false;
        return true;
    }

    inline int init_quanta(lua_State* L, const char* fconf) {
        //初始化
        std::string err;
        const char* args[2]{ "quanta", fconf };
        //初始化
        if (!q_app.setup(L, 2, args, err)) {
            lua_pushboolean(L, false);
            lua_pushstring(L, err.c_str());
            return 2;
        }
        lua_pushboolean(L, true);
        return 1;
    }

    luakit::lua_table open_quanta(lua_State* L) {
        kit_state kit_state(L);
        lua_table lquanta = kit_state.new_table("quanta");
        lquanta.set_function("init_quanta", init_quanta);
        return lquanta;
    }
}

extern "C" {
    LUALIB_API int luaopen_quanta(lua_State* L) {
        auto lquanta = quanta::open_quanta(L);
        return lquanta.push_stack();
    }
}
