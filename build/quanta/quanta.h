#pragma once
#include <map>
#include <string>
#include <functional>

#include "lua_kit.h"

using namespace luakit;

namespace quanta {

    class quanta_app final {
    public:
        ~quanta_app() {
            m_lua->close();
            delete m_lua;
        }
        bool init(std::string& serr);
        bool setup(lua_State* L, int argc, const char* argv[], std::string& serr);
        void set_env(const char* key, const char* value, int over = 0);

        luakit::kit_state* state() { return m_lua; };
        
        lua_State* L() { return m_lua->L();  }

    protected:
        const char* get_env(const char* key);

    private:
        uint64_t m_signal = 0;
        luakit::kit_state* m_lua;
        std::map<std::string, std::string> m_environs;
    };
}
