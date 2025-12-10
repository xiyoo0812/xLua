#pragma once

#include "lua_kit.h"

class quanta_app final
{
public:
    ~quanta_app();
    
    void run();
    bool step();
    bool init();
    bool load(int argc, const char* argv[]);
    void set_signal(uint32_t n, bool b = true);
    void add_path(const char* field, const char* path);
    void set_env(const char* key, const char* value, int over = 0);
    bool setup(int argc, const char* argv[], lua_State* L = nullptr);

    luakit::kit_state* state() { return m_lua; };
    
    lua_State* L() { return m_lua->L();  }
    std::string last_error() { return m_error;  }

protected:
    const char* get_env(const char* key);
    template<typename... Args>
    void exception_handler(std::string_view msg, Args&&... args){
        m_error = std::vformat(msg, std::make_format_args(args...)).c_str();
        printf(m_error.c_str());
    }

private:
    uint64_t m_signal = 0;
    std::string m_error = "";
    luakit::kit_state* m_lua = nullptr;
    std::unordered_map<std::string, std::string> m_environs;
};

extern quanta_app* g_app;
