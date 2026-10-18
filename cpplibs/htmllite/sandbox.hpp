#pragma once

extern "C" {
    #include <luajit-2.1/lua.h>
    #include <luajit-2.1/lauxlib.h>
    #include <luajit-2.1/lualib.h>
}

#include <string>

class LuaSandbox {
public:
    LuaSandbox();
    ~LuaSandbox();

    // Registrar funciones seguras de la interfaz (ej. API de UI)
    void register_c_function(const char* name, lua_CFunction func);

    // Ejecutar código Lua en cadena (para etiquetas <script> en línea)
    bool run_string(const std::string& code, std::string& error_out);

    // Ejecutar archivo Lua externo (para etiquetas <script src="script.lua">)
    bool run_file(const std::string& filepath, std::string& error_out);

private:
    lua_State* m_L;
    void init_sandbox_environment();
};