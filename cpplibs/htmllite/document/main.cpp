#include "signatures.hpp"

// Función exportada para ser llamada desde el sandbox
extern "C" void register_document_api(lua_State* L) {
    lua_newtable(L); // Crear la tabla principal 'document'

    // Registrar métodos
    lua_pushcfunction(L, l_document_get_element_by_id);
    lua_setfield(L, -2, "getElementById");

    lua_pushcfunction(L, l_document_query_selector);
    lua_setfield(L, -2, "querySelector");

    // Inyectar globalmente como 'document' en el entorno aislado
    lua_setglobal(L, "document");
}