#pragma once

extern "C" {
    #include <luajit-2.1/lua.h>
    #include <luajit-2.1/lauxlib.h>
}

// Declaraciones de métodos del objeto document
int l_document_get_element_by_id(lua_State* L);
int l_document_query_selector(lua_State* L);