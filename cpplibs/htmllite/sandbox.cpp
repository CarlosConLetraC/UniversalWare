#include "sandbox.hpp"
#include <iostream>

// Simulación de document.getElementById para el DOM de litehtml
static int l_document_get_element_by_id(lua_State* L) {
    const char* id = luaL_checkstring(L, 1);
    
    // Aquí puedes consultar el árbol DOM real administrado por tu HtmlContainer
    std::cout << "[DOM API] Buscando elemento por ID: " << id << std::endl;

    // Retornamos un objeto (tabla proxy) que represente al elemento del DOM
    lua_newtable(L);
    lua_pushstring(L, id);
    lua_setfield(L, -2, "id");
    
    // Añadir propiedades o métodos simulados (ej. innerText, value)
    lua_pushstring(L, "");
    lua_setfield(L, -2, "value");

    return 1; // Retorna el objeto elemento a Lua
}

static int l_document_query_selector(lua_State* L) {
    const char* selector = luaL_checkstring(L, 1);
    std::cout << "[DOM API] Ejecutando querySelector con selector: " << selector << std::endl;

    // Retornar un elemento simulado o nil si no se encuentra
    lua_newtable(L);
    lua_pushstring(L, selector);
    lua_setfield(L, -2, "selector");
    return 1;
}

// Declarar la función externa proveniente de document/main.cpp
extern "C" void register_document_api(lua_State* L);

void LuaSandbox::init_sandbox_environment() {
    // 1. Cargar librerías base seguras
    lua_pushcfunction(m_L, luaopen_base);  lua_call(m_L, 0, 0);
    lua_pushcfunction(m_L, luaopen_math);  lua_call(m_L, 0, 0);
    lua_pushcfunction(m_L, luaopen_string); lua_call(m_L, 0, 0);
    lua_pushcfunction(m_L, luaopen_table);  lua_call(m_L, 0, 0);

    // 2. Bloquear funciones peligrosas del sistema operativo e I/O
    lua_pushnil(m_L); lua_setglobal(m_L, "io");
    lua_pushnil(m_L); lua_setglobal(m_L, "os");
    lua_pushnil(m_L); lua_setglobal(m_L, "package");
    lua_pushnil(m_L); lua_setglobal(m_L, "debug");

    // 3. Inyectar la API del DOM modularizada
    register_document_api(m_L);
}