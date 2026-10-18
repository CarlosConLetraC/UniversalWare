#include "signatures.hpp"
#include <iostream>

int l_document_get_element_by_id(lua_State* L) {
    const char* id = luaL_checkstring(L, 1);
    // std::cout << "[DOM API] Buscando elemento por ID: " << id << std::endl;

    // Retornar un objeto de elemento simulado (tabla proxy)
    lua_newtable(L);
    lua_pushstring(L, id);
    lua_setfield(L, -2, "id");
    
    lua_pushstring(L, "");
    lua_setfield(L, -2, "value");

    return 1;
}

int l_document_query_selector(lua_State* L) {
    const char* selector = luaL_checkstring(L, 1);
    // std::cout << "[DOM API] Ejecutando querySelector con selector: " << selector << std::endl;

    lua_newtable(L);
    lua_pushstring(L, selector);
    lua_setfield(L, -2, "selector");
    return 1;
}