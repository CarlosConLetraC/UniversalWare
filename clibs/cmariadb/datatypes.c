#include "datatypes.h"

// Función auxiliar para obtener y validar un SqlValue sin importar si es geométrica o general
SqlValue* check_sql_value(lua_State *L, int index) {
    void *ud = lua_touserdata(L, index);
    if (ud != NULL) {
        if (lua_getmetatable(L, index)) {
            // Comparamos si la metatabla del objeto coincide con cualquiera de las dos permitidas
            lua_getfield(L, LUA_REGISTRYINDEX, SQLVALUE_META);
            lua_getfield(L, LUA_REGISTRYINDEX, SQLVALUE_GEOM_META);
            
            int is_valid = 0;
            if (lua_rawequal(L, -1, -3) || lua_rawequal(L, -2, -3))
                is_valid = 1;
            lua_pop(L, 3); // Limpiamos la pila
            
            if (is_valid) return (SqlValue *)ud;
        }
    }
    luaL_typerror(L, index, SQLVALUE_META);
    return NULL;
}

// 1. Destructor / Recolector de basura (Liberación segura de memoria dinámica y blobs/geometría)
static int sqlvalue_gc(lua_State *L) {
    // Obtenemos el userdata de forma segura permitiendo ambas metatablas
    SqlValue *v = check_sql_value(L, 1);
    if (!v) return 0;

    if (!v->is_null) {
        switch (v->sql_type) {
            case MYSQL_TYPE_VAR_STRING:
            case MYSQL_TYPE_STRING:
            case MYSQL_TYPE_VARCHAR:
            case MYSQL_TYPE_JSON:
            case MYSQL_TYPE_ENUM:
            case MYSQL_TYPE_SET:
            case MYSQL_TYPE_DATE:
            case MYSQL_TYPE_DATETIME:
            case MYSQL_TYPE_TIME:
            case MYSQL_TYPE_TIMESTAMP:
            case MYSQL_TYPE_TINY_BLOB:
            case MYSQL_TYPE_MEDIUM_BLOB:
            case MYSQL_TYPE_LONG_BLOB:
            case MYSQL_TYPE_BLOB:
                if (v->data.string_val.ptr) {
                    free(v->data.string_val.ptr);
                    v->data.string_val.ptr = NULL;
                }
                break;
            case MYSQL_TYPE_GEOMETRY:
                if (v->data.blob_val.ptr) {
                    free((void *)v->data.blob_val.ptr);
                    v->data.blob_val.ptr = NULL;
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

// 2. Metamétodo __tostring (Devuelve cadenas formateadas para la consulta SQL o representación)
static int sqlvalue_tostring(lua_State *L) {
    SqlValue *v = check_sql_value(L, 1);
    if (v->is_null) {
        lua_pushstring(L, "NULL");
        return 1;
    }

    switch (v->sql_type) {
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_BIT: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%lld", (long long)v->data.int_val);
            lua_pushstring(L, buffer);
            break;
        }
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_BOOL: {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%d", (int)v->data.int_val);
            lua_pushstring(L, buffer);
            break;
        }
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_DECIMAL: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%f", v->data.float_val);
            lua_pushstring(L, buffer);
            break;
        }
        case MYSQL_TYPE_GEOMETRY: {
            if (v->data.blob_val.ptr)
                lua_pushlstring(L, (const char *)v->data.blob_val.ptr, v->data.blob_val.len);
            else
                lua_pushstring(L, "");
            break;
        }
        default: {
            if (v->data.string_val.ptr)
                lua_pushlstring(L, v->data.string_val.ptr, v->data.string_val.len);
            else
                lua_pushstring(L, "");
            break;
        }
    }
    return 1;
}

// 3. Método tonumber()
static int sqlvalue_tonumber(lua_State *L) {
    SqlValue *v = check_sql_value(L, 1);
    if (v->is_null) {
        lua_pushnil(L);
        return 1;
    }
    switch (v->sql_type) {
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_BIT:
        case MYSQL_TYPE_BOOL:
            lua_pushnumber(L, (lua_Number)v->data.int_val);
            break;
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_DECIMAL:
            lua_pushnumber(L, v->data.float_val);
            break;
        case MYSQL_TYPE_GEOMETRY:
            lua_pushnumber(L, 0.0);
            break;
        default:
            lua_pushnumber(L, v->data.string_val.ptr ? atof(v->data.string_val.ptr) : 0.0);
            break;
    }
    return 1;
}

// 4. Método type() (Mapeo completo de los tipos de MariaDB)
static int sqlvalue_type(lua_State *L) {
    SqlValue *v = check_sql_value(L, 1);
    switch (v->sql_type) {
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            lua_pushstring(L, "DECIMAL");
            break;
        case MYSQL_TYPE_TINY:
            lua_pushstring(L, "TINYINT");
            break;
        case MYSQL_TYPE_SHORT:
            lua_pushstring(L, "SMALLINT");
            break;
        case MYSQL_TYPE_INT24:
            lua_pushstring(L, "MEDIUMINT");
            break;
        case MYSQL_TYPE_LONG:
            lua_pushstring(L, "INTEGER");
            break;
        case MYSQL_TYPE_LONGLONG:
            lua_pushstring(L, "BIGINT");
            break;
        case MYSQL_TYPE_YEAR:
            lua_pushstring(L, "YEAR");
            break;
        case MYSQL_TYPE_BIT:
            lua_pushstring(L, "BIT");
            break;
        case MYSQL_TYPE_BOOL:
            lua_pushstring(L, "BOOLEAN");
            break;
        case MYSQL_TYPE_FLOAT:
            lua_pushstring(L, "FLOAT");
            break;
        case MYSQL_TYPE_DOUBLE:
            lua_pushstring(L, "DOUBLE");
            break;
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_STRING:
            lua_pushstring(L, "STRING");
            break;
        case MYSQL_TYPE_JSON:
            lua_pushstring(L, "JSON");
            break;
        case MYSQL_TYPE_ENUM:
            lua_pushstring(L, "ENUM");
            break;
        case MYSQL_TYPE_SET:
            lua_pushstring(L, "SET");
            break;
        case MYSQL_TYPE_DATE:
            lua_pushstring(L, "DATE");
            break;
        case MYSQL_TYPE_DATETIME:
            lua_pushstring(L, "DATETIME");
            break;
        case MYSQL_TYPE_TIME:
            lua_pushstring(L, "TIME");
            break;
        case MYSQL_TYPE_TIMESTAMP:
            lua_pushstring(L, "TIMESTAMP");
            break;
        case MYSQL_TYPE_GEOMETRY:
            lua_pushstring(L, "GEOMETRY");
            break;
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
            lua_pushstring(L, "BLOB");
            break;
        default:
            lua_pushstring(L, "UNKNOWN");
            break;
    }
    return 1;
}

// Método unificado de extracción de valor polimórfico (Estático y privado al módulo)
static int sqlvalue_value(lua_State *L) {
    SqlValue *v = check_sql_value(L, 1);
    if (v->is_null) {
        lua_pushnil(L);
        return 1;
    }

    switch (v->sql_type) {
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_BIT:
        case MYSQL_TYPE_BOOL:
            lua_pushinteger(L, v->data.int_val);
            break;
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_DECIMAL:
            lua_pushnumber(L, v->data.float_val);
            break;
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_JSON:
            if (v->data.string_val.ptr)
                lua_pushlstring(L, v->data.string_val.ptr, v->data.string_val.len);
            else
                lua_pushstring(L, "");
            break;
        case MYSQL_TYPE_GEOMETRY: {
            MariaDBGeometry geom;
            if (parse_internal_geometry(v->data.blob_val.ptr, v->data.blob_val.len, &geom) == 0 ||
                parse_wkb_geometry(v->data.blob_val.ptr, v->data.blob_val.len, &geom) == 0) {
                
                lua_newtable(L);
                lua_pushinteger(L, geom.srid); lua_setfield(L, -2, "srid");
                lua_pushinteger(L, geom.byte_order); lua_setfield(L, -2, "byte_order");
                lua_pushinteger(L, geom.type); lua_setfield(L, -2, "type");
                if (geom.coordinates && geom.coordinate_len > 0) {
                    lua_pushlstring(L, (const char *)geom.coordinates, geom.coordinate_len);
                    lua_setfield(L, -2, "coordinates");
                    free(geom.coordinates);
                } else {
                    lua_pushnil(L); lua_setfield(L, -2, "coordinates");
                }
            } else {
                lua_pushnil(L);
            }
            break;
        }
        default:
            lua_pushnil(L);
            break;
    }
    return 1;
}

/* Método de instancia para decodificar geometría espacial en Lua */
static int sqlvalue_get_geometry(lua_State *L) {
    SqlValue *v = check_sql_value(L, 1);
    if (v->is_null || v->sql_type != MYSQL_TYPE_GEOMETRY) {
        lua_pushnil(L);
        return 1;
    }

    MariaDBGeometry geom;
    /* Intentar parsear el formato interno de MariaDB (4 bytes SRID + WKB) */
    if (parse_internal_geometry(v->data.blob_val.ptr, v->data.blob_val.len, &geom) != 0) {
        /* Fallback si el formato fuera WKB puro sin prefijo SRID */
        if (parse_wkb_geometry(v->data.blob_val.ptr, v->data.blob_val.len, &geom) != 0) {
            lua_pushnil(L);
            return 1;
        }
    }

    /* Crear una tabla Lua con los datos espaciales decodificados */
    lua_newtable(L);
    lua_pushinteger(L, geom.srid);
    lua_setfield(L, -2, "srid");

    lua_pushinteger(L, geom.byte_order);
    lua_setfield(L, -2, "byte_order");

    lua_pushinteger(L, geom.type);
    lua_setfield(L, -2, "type");

    /* Devolver el payload crudo de coordenadas como un string binario */
    if (geom.coordinates && geom.coordinate_len > 0) {
        lua_pushlstring(L, (const char *)geom.coordinates, geom.coordinate_len);
        lua_setfield(L, -2, "coordinates");
        free(geom.coordinates); /* Liberar la memoria temporal reservada por el parser */
    } else {
        lua_pushnil(L);
        lua_setfield(L, -2, "coordinates");
    }

    return 1;
}

// Función auxiliar pura para parsear recursivamente cualquier sub-geometría binaria
static void push_geometry_to_lua(lua_State *L, const unsigned char **cursor, const unsigned char *end_ptr) {
    if (*cursor + 5 > end_ptr) return;

    uint8_t byte_order = **cursor;
    uint32_t type;
    memcpy(&type, *cursor + 1, 4);
    *cursor += 5;

    switch (type) {
        case GEOM_TYPE_POINT: {
            if (*cursor + 16 > end_ptr) return;
            double x, y;
            memcpy(&x, *cursor, 8);
            memcpy(&y, *cursor + 8, 8);
            *cursor += 16;

            lua_newtable(L);
            lua_pushnumber(L, x); lua_setfield(L, -2, "x");
            lua_pushnumber(L, y); lua_setfield(L, -2, "y");
            break;
        }
        case GEOM_TYPE_LINESTRING:
        case GEOM_TYPE_MULTIPOINT: {
            if (*cursor + 4 > end_ptr) return;
            uint32_t num_points;
            memcpy(&num_points, *cursor, 4);
            *cursor += 4;

            lua_newtable(L);
            for (uint32_t i = 0; i < num_points; i++) {
                if (*cursor + 16 > end_ptr) break;
                double x, y;
                memcpy(&x, *cursor, 8);
                memcpy(&y, *cursor + 8, 8);
                *cursor += 16;

                lua_newtable(L);
                lua_pushnumber(L, x); lua_setfield(L, -2, "x");
                lua_pushnumber(L, y); lua_setfield(L, -2, "y");
                lua_rawseti(L, -2, i + 1);
            }
            break;
        }
        case GEOM_TYPE_POLYGON:
        case GEOM_TYPE_MULTILINESTRING: {
            if (*cursor + 4 > end_ptr) return;
            uint32_t num_rings;
            memcpy(&num_rings, *cursor, 4);
            *cursor += 4;

            lua_newtable(L);
            for (uint32_t i = 0; i < num_rings; i++) {
                if (*cursor + 4 > end_ptr) break;
                uint32_t num_points;
                memcpy(&num_points, *cursor, 4);
                *cursor += 4;

                lua_newtable(L);
                for (uint32_t j = 0; j < num_points; j++) {
                    if (*cursor + 16 > end_ptr) break;
                    double x, y;
                    memcpy(&x, *cursor, 8);
                    memcpy(&y, *cursor + 8, 8);
                    *cursor += 16;

                    lua_newtable(L);
                    lua_pushnumber(L, x); lua_setfield(L, -2, "x");
                    lua_pushnumber(L, y); lua_setfield(L, -2, "y");
                    lua_rawseti(L, -2, j + 1);
                }
                lua_rawseti(L, -2, i + 1);
            }
            break;
        }
        case GEOM_TYPE_GEOMETRYCOLLECTION: {
            if (*cursor + 4 > end_ptr) return;
            uint32_t num_geoms;
            memcpy(&num_geoms, *cursor, 4);
            *cursor += 4;

            lua_newtable(L);
            for (uint32_t i = 0; i < num_geoms; i++) {
                push_geometry_to_lua(L, cursor, end_ptr);
                lua_rawseti(L, -2, i + 1);
            }
            break;
        }
        default:
            lua_pushnil(L);
            break;
    }
}

/* Método de instancia: row.col_geometry:coordinates() -> Devuelve una estructura limpia en Lua */
static int sqlvalue_coordinates(lua_State *L) {
    SqlValue *v = check_sql_value(L, 1); // (SqlValue *)luaL_checkudata(L, 1, SQLVALUE_META);
    if (v->is_null || v->sql_type != MYSQL_TYPE_GEOMETRY) {
        lua_pushnil(L);
        return 1;
    }

    MariaDBGeometry geom;
    int parsed = -1;

    // Verificar si el blob es una cadena de texto WKT en lugar de binario
    if (v->data.blob_val.len > 0 && isalpha((unsigned char)((char *)v->data.blob_val.ptr)[0])) {
        // Crear una copia temporal terminada en null para parsear WKT de forma segura
        char *wkt_str = malloc(v->data.blob_val.len + 1);
        memcpy(wkt_str, v->data.blob_val.ptr, v->data.blob_val.len);
        wkt_str[v->data.blob_val.len] = '\0';
        
        parsed = parse_wkt_to_geometry(wkt_str, &geom);
        // printf("DEBUG: parse_wkt_to_geometry retornó: %d\n", parsed);
        // printf("DEBUG: geom.type: %d\n", geom.type);
        // printf("DEBUG: geom.coordinate_len: %zu\n", geom.coordinate_len);

        free(wkt_str);
    } else {
        // Intento normal con binarios WKB / MariaDB
        parsed = parse_internal_geometry(v->data.blob_val.ptr, v->data.blob_val.len, &geom);
        if (parsed != 0)
            parsed = parse_wkb_geometry(v->data.blob_val.ptr, v->data.blob_val.len, &geom);
    }

    if (parsed != 0) {
        lua_pushnil(L);
        return 1;
    }

    switch (geom.type) {
        case GEOM_TYPE_POINT: {
            if (geom.coordinates && geom.coordinate_len >= 16) {
                double *coords = (double *)geom.coordinates;
                // printf("DEBUG: coords[0] (X): %f\n", coords[0]);
                // printf("DEBUG: coords[1] (Y): %f\n", coords[1]);
                lua_newtable(L);
                lua_pushnumber(L, coords[0]); lua_setfield(L, -2, "x");
                lua_pushnumber(L, coords[1]); lua_setfield(L, -2, "y");
                free(geom.coordinates);
                return 1;
            } /*else {
                printf("DEBUG: geom.coordinates es NULL o menor a 16 bytes.\n");
            }*/
            break;
        }

        case GEOM_TYPE_LINESTRING:
        case GEOM_TYPE_MULTIPOINT: {
            /* LineString y MultiPoint comparten una estructura similar de lista de puntos:
             * - 4 bytes: uint32_t numPoints
             * - Seguido de numPoints pares de doubles (X, Y)
             */
            if (geom.coordinates && geom.coordinate_len >= 4) {
                uint32_t num_points;
                memcpy(&num_points, geom.coordinates, 4);
                double *pts = (double *)(geom.coordinates + 4);
                
                lua_newtable(L);
                for (uint32_t i = 0; i < num_points; i++) {
                    lua_newtable(L);
                    lua_pushnumber(L, pts[i * 2]);     lua_setfield(L, -2, "x");
                    lua_pushnumber(L, pts[i * 2 + 1]); lua_setfield(L, -2, "y");
                    lua_rawseti(L, -2, i + 1);
                }
                free(geom.coordinates);
                return 1;
            }
            break;
        }

        case GEOM_TYPE_POLYGON:
        case GEOM_TYPE_MULTILINESTRING: {
            /* Polygon y MultiLineString manejan una lista de anillos o sub-líneas:
             * - 4 bytes: uint32_t numRings / numLineStrings
             * - Cada anillo/línea contiene a su vez un contador de puntos de 4 bytes seguido de sus coordenadas.
             */
            unsigned char *ptr = geom.coordinates;
            uint32_t num_elements;
            memcpy(&num_elements, ptr, 4);
            ptr += 4;

            lua_newtable(L); // Contenedor principal de anillos/líneas
            for (uint32_t i = 0; i < num_elements; i++) {
                uint32_t num_points;
                memcpy(&num_points, ptr, 4);
                ptr += 4;

                double *pts = (double *)ptr;
                lua_newtable(L); // Tabla para este anillo/línea en específico
                for (uint32_t j = 0; j < num_points; j++) {
                    lua_newtable(L);
                    lua_pushnumber(L, pts[j * 2]);     lua_setfield(L, -2, "x");
                    lua_pushnumber(L, pts[j * 2 + 1]); lua_setfield(L, -2, "y");
                    lua_rawseti(L, -2, j + 1);
                    
                    ptr += 16; // Avanzamos 16 bytes por cada punto (dos doubles de 8 bytes)
                }
                lua_rawseti(L, -2, i + 1);
            }
            free(geom.coordinates);
            return 1;
        }

        case GEOM_TYPE_MULTIPOLYGON: {
            /* MultiPolygon contiene múltiples polígonos, donde cada polígono contiene anillos */
            unsigned char *ptr = geom.coordinates;
            uint32_t num_polygons;
            memcpy(&num_polygons, ptr, 4);
            ptr += 4;

            lua_newtable(L);
            for (uint32_t i = 0; i < num_polygons; i++) {
                uint32_t num_rings;
                memcpy(&num_rings, ptr, 4);
                ptr += 4;

                lua_newtable(L); // Contenedor de anillos para este polígono
                for (uint32_t r = 0; r < num_rings; r++) {
                    uint32_t num_points;
                    memcpy(&num_points, ptr, 4);
                    ptr += 4;

                    double *pts = (double *)ptr;
                    lua_newtable(L); // Puntos del anillo
                    for (uint32_t j = 0; j < num_points; j++) {
                        lua_newtable(L);
                        lua_pushnumber(L, pts[j * 2]);     lua_setfield(L, -2, "x");
                        lua_pushnumber(L, pts[j * 2 + 1]); lua_setfield(L, -2, "y");
                        lua_rawseti(L, -2, j + 1);
                        ptr += 16;
                    }
                    lua_rawseti(L, -2, r + 1);
                }
                lua_rawseti(L, -2, i + 1);
            }
            free(geom.coordinates);
            return 1;
        }

        case GEOM_TYPE_GEOMETRYCOLLECTION: {
            if (geom.coordinates && geom.coordinate_len >= 4) {
                uint32_t num_geoms;
                memcpy(&num_geoms, geom.coordinates, 4);
                const unsigned char *cursor = geom.coordinates + 4;
                const unsigned char *end_ptr = geom.coordinates + geom.coordinate_len;

                lua_newtable(L);
                for (uint32_t i = 0; i < num_geoms; i++) {
                    push_geometry_to_lua(L, &cursor, end_ptr);
                    lua_rawseti(L, -2, i + 1);
                }
                free(geom.coordinates);
                return 1;
            }
            break;
        }

        case GEOM_TYPE_UNKNOWN:
        default:
            break;
    }

    if (geom.coordinates)
        free(geom.coordinates);

    lua_pushnil(L);
    return 1;
}

// 5. Registro de la metatabla y sus métodos de instancia
void register_sqlvalue_meta(lua_State *L) {
    // --- 1. Metatabla General (para string, int, float, etc.) ---
    luaL_newmetatable(L, SQLVALUE_META);

    lua_pushcfunction(L, sqlvalue_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, sqlvalue_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, sqlvalue_tonumber);
    lua_setfield(L, -2, "tonumber");

    lua_pushcfunction(L, sqlvalue_type);
    lua_setfield(L, -2, "type");

    lua_pushcfunction(L, sqlvalue_value);
    lua_setfield(L, -2, "value");

    lua_pushcfunction(L, sqlvalue_tostring);
    lua_setfield(L, -2, "tostring");

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1); // Cierra metatabla general

    // --- 2. Metatabla de Geometrías (exclusiva para datos espaciales) ---
    luaL_newmetatable(L, SQLVALUE_GEOM_META);

    // Copiamos o reasignamos los métodos base comunes si lo deseas, 
    // o puedes hacer que __index apunte a la metatabla general para reutilizar string/gc/value.
    // Una forma elegante en C es configurar su __index para que busque primero en la metatabla geom 
    // y si no está, delegue en la general, O simplemente registrar los métodos básicos de nuevo:
    
    lua_pushcfunction(L, sqlvalue_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, sqlvalue_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, sqlvalue_tonumber);
    lua_setfield(L, -2, "tonumber");

    lua_pushcfunction(L, sqlvalue_type);
    lua_setfield(L, -2, "type");

    lua_pushcfunction(L, sqlvalue_value);
    lua_setfield(L, -2, "value");

    lua_pushcfunction(L, sqlvalue_tostring);
    lua_setfield(L, -2, "tostring");

    /* Métodos espaciales exclusivos */
    lua_pushcfunction(L, sqlvalue_get_geometry);
    lua_setfield(L, -2, "getgeometry");

    lua_pushcfunction(L, sqlvalue_coordinates);
    lua_setfield(L, -2, "castcoordinates");

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1); // Cierra metatabla geom
}

// 6. Función base constructora robusta ampliada
SqlValue* l_sqlvalue_create_impl(lua_State *L, enum enum_field_types type, int is_int) {
    int has_arg = (lua_gettop(L) >= 1 && !lua_isnil(L, 1));

    SqlValue *v = (SqlValue *)lua_newuserdata(L, sizeof(SqlValue));
    v->sql_type = type;
    v->is_null = !has_arg ? 1 : 0;
    memset(&v->data, 0, sizeof(v->data));

    if (v->is_null) goto set_metatable;

    /* 1. Rama de Enteros, Bit, Bool y Year */
    if (is_int || type == MYSQL_TYPE_YEAR || type == MYSQL_TYPE_BIT || type == MYSQL_TYPE_BOOL) {
        if (lua_type(L, 1) == 10) { // LUA_TCDATA de LuaJIT
            int64_t *pval = (int64_t *)lua_topointer(L, 1);
            v->data.int_val = pval ? *pval : 0;
        } else {
            v->data.int_val = (int64_t)lua_tonumber(L, 1);
        }
        goto set_metatable;
    }

    /* 2. Rama de Flotantes y Decimales */
    if (type == MYSQL_TYPE_FLOAT || type == MYSQL_TYPE_DOUBLE || type == MYSQL_TYPE_NEWDECIMAL || type == MYSQL_TYPE_DECIMAL) {
        v->data.float_val = (double)lua_tonumber(L, 1);
        goto set_metatable;
    }

    /* 3. Rama de Geometría / Binarios */
    if (type == MYSQL_TYPE_GEOMETRY) {
        size_t len = 0;
        const char *str = lua_tolstring(L, 1, &len);
        if (!str) goto set_metatable;

        unsigned char *buf = (unsigned char *)malloc(len);
        if (buf) {
            memcpy(buf, str, len);
            v->data.blob_val.ptr = buf;
            v->data.blob_val.len = len;
        }
        goto set_metatable;
    }

    /* 4. Rama por defecto (Cadenas, Textos, JSON, etc.) */
    size_t len = 0;
    const char *str = lua_tolstring(L, 1, &len);
    if (str) {
        v->data.string_val.ptr = (char *)malloc(len + 1);
        if (v->data.string_val.ptr) {
            memcpy(v->data.string_val.ptr, str, len);
            v->data.string_val.ptr[len] = '\0';
            v->data.string_val.len = len;
        }
    } else {
        v->data.string_val.ptr = strdup("");
        v->data.string_val.len = 0;
    }

    set_metatable:
        luaL_getmetatable(L, SQLVALUE_META);
        lua_setmetatable(L, -2);
        return v;
}

// 7. Mapeador de campos recibidos desde MariaDB (Lectura con cobertura total)
void push_mariadb_field(lua_State *L, MYSQL_FIELD *field, char *val, unsigned long length) {
    SqlValue *v = (SqlValue *)lua_newuserdata(L, sizeof(SqlValue));
    v->sql_type = field->type;
    memset(&v->data, 0, sizeof(v->data));

    if (!val) {
        v->is_null = 1;
    } else {
        v->is_null = 0;
        switch (field->type) {
            case MYSQL_TYPE_TINY:
            case MYSQL_TYPE_SHORT:
            case MYSQL_TYPE_LONG:
            case MYSQL_TYPE_INT24:
            case MYSQL_TYPE_LONGLONG:
            case MYSQL_TYPE_YEAR:
            case MYSQL_TYPE_BIT:
            case MYSQL_TYPE_BOOL:
                v->data.int_val = strtoll(val, NULL, 10);
                break;

            case MYSQL_TYPE_FLOAT:
            case MYSQL_TYPE_DOUBLE:
            case MYSQL_TYPE_DECIMAL:
            case MYSQL_TYPE_NEWDECIMAL:
                v->data.float_val = strtod(val, NULL);
                break;

            case MYSQL_TYPE_GEOMETRY:
                v->data.blob_val.ptr = (unsigned char *)malloc(length);
                if (v->data.blob_val.ptr) {
                    memcpy((void *)v->data.blob_val.ptr, val, length);
                    v->data.blob_val.len = length;
                } else {
                    v->data.blob_val.len = 0;
                }
                break;

            default:
                v->data.string_val.ptr = (char *)malloc(length + 1);
                if (v->data.string_val.ptr) {
                    memcpy(v->data.string_val.ptr, val, length);
                    v->data.string_val.ptr[length] = '\0';
                    v->data.string_val.len = length;
                } else {
                    v->data.string_val.len = 0;
                }
                break;
        }
    }

    luaL_getmetatable(L, SQLVALUE_META);
    lua_setmetatable(L, -2);
}

static void init_point_list(WKTPointList *list) {
    list->count = 0;
    list->capacity = 8;
    list->points = malloc(list->capacity * sizeof(WKTPoint));
}

static void add_point(WKTPointList *list, double x, double y) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->points = realloc(list->points, list->capacity * sizeof(WKTPoint));
    }
    list->points[list->count++] = (WKTPoint){x, y};
}

static void free_point_list(WKTPointList *list) {
    free(list->points);
}

int parse_wkt_to_geometry(const char *wkt_str, MariaDBGeometry *geom) {
    if (!wkt_str || !geom) return -1;
    
    geom->srid = 0;
    geom->byte_order = 1; /* Little-Endian */
    
    // Omitir espacios iniciales
    // while (*wkt_str && isspace((unsigned char)*wkt_str)) wkt_str++;
    for (; *wkt_str && isspace((unsigned char)*wkt_str); wkt_str++);

    if (strncmp(wkt_str, "POINT", 5) == 0) {
        geom->type = GEOM_TYPE_POINT;
        const char *p = strchr(wkt_str, '(');
        if (!p) return -1;
        p++;

        char *endptr;
        double x = strtod(p, &endptr);
        if (p == endptr) return -1;
        p = endptr;
        double y = strtod(p, &endptr);
        if (p == endptr) return -1;

        geom->coordinate_len = 16;
        geom->coordinates = malloc(16);
        if (!geom->coordinates) return -1;
        memcpy(geom->coordinates, &x, 8);
        memcpy(geom->coordinates + 8, &y, 8);
        return 0;
    }
    else if (strncmp(wkt_str, "LINESTRING", 10) == 0 || strncmp(wkt_str, "MULTIPOINT", 10) == 0) {
        geom->type = (strncmp(wkt_str, "LINESTRING", 10) == 0) ? GEOM_TYPE_LINESTRING : GEOM_TYPE_MULTIPOINT;
        const char *p = strchr(wkt_str, '(');
        if (!p) return -1;
        p++;

        WKTPointList plist;
        init_point_list(&plist);

        while (*p && *p != ')') {
            // while (*p && (isspace((unsigned char)*p) || *p == ',' || *p == '(')) p++;
            for(; *p && (isspace((unsigned char)*p) || *p == ',' || *p == '('); p++);
            if (*p == ')' || !*p) break;

            char *endptr;
            double x = strtod(p, &endptr);
            if (p == endptr) { free_point_list(&plist); return -1; }
            p = endptr;

            // while (*p && isspace((unsigned char)*p)) p++;
            for (; *p && isspace((unsigned char)*p); p++);
            double y = strtod(p, &endptr);
            if (p == endptr) { free_point_list(&plist); return -1; }
            p = endptr;

            add_point(&plist, x, y);
        }

        // Estructura binaria: 4 bytes (numPoints) + (numPoints * 16 bytes)
        size_t total_len = 4 + (plist.count * 16);
        geom->coordinate_len = total_len;
        geom->coordinates = malloc(total_len);
        if (!geom->coordinates) { free_point_list(&plist); return -1; }

        uint32_t num_points = (uint32_t)plist.count;
        memcpy(geom->coordinates, &num_points, 4);

        unsigned char *dst = geom->coordinates + 4;
        for (size_t i = 0; i < plist.count; i++) {
            memcpy(dst, &plist.points[i].x, 8);
            memcpy(dst + 8, &plist.points[i].y, 8);
            dst += 16;
        }

        free_point_list(&plist);
        return 0;
    }
    else if (strncmp(wkt_str, "POLYGON", 7) == 0 || strncmp(wkt_str, "MULTILINESTRING", 15) == 0) {
        geom->type = (strncmp(wkt_str, "POLYGON", 7) == 0) ? GEOM_TYPE_POLYGON : GEOM_TYPE_MULTILINESTRING;
        const char *p = strchr(wkt_str, '(');
        if (!p) return -1;
        p++;

        // Contenedor dinámico para manejar múltiples anillos/sub-líneas
        typedef struct {
            WKTPoint *pts;
            size_t count;
        } Ring;

        size_t ring_cap = 4, ring_count = 0;
        Ring *rings = malloc(ring_cap * sizeof(Ring));

        while (*p && *p != ')') {
            // while (*p && (*p == '(' || isspace((unsigned char)*p) || *p == ',')) p++;
            for(; *p && (*p == '(' || isspace((unsigned char)*p) || *p == ','); p++);
            if (*p == ')' || !*p) break;

            WKTPointList plist;
            init_point_list(&plist);

            while (*p && *p != ')') {
                // while (*p && (isspace((unsigned char)*p) || *p == ',' || *p == '(')) p++;
                for (; *p && (isspace((unsigned char)*p) || *p == ',' || *p == '('); p++);
                if (*p == ')' || !*p) break;

                char *endptr;
                double x = strtod(p, &endptr);
                if (p == endptr) break;
                p = endptr;

                // while (*p && isspace((unsigned char)*p)) p++;
                for(; *p && isspace((unsigned char)*p); p++);
                double y = strtod(p, &endptr);
                if (p == endptr) break;
                p = endptr;

                add_point(&plist, x, y);
            }
            // while (*p && (*p == ')' || *p == ',' || isspace((unsigned char)*p))) p++;
            for (; *p && (*p == ')' || *p == ',' || isspace((unsigned char)*p)); p++);

            if (ring_count >= ring_cap) {
                ring_cap *= 2;
                rings = realloc(rings, ring_cap * sizeof(Ring));
            }
            rings[ring_count++] = (Ring){plist.points, plist.count};
        }

        // Calcular tamaño total: 4 bytes (numRings) + por cada anillo: [4 bytes (numPoints) + puntos]
        size_t total_len = 4;
        for (size_t i = 0; i < ring_count; i++)
            total_len += 4 + (rings[i].count * 16);

        geom->coordinate_len = total_len;
        geom->coordinates = malloc(total_len);
        if (!geom->coordinates) {
            for(size_t i=0; i<ring_count; i++) free(rings[i].pts);
            free(rings);
            return -1;
        }

        uint32_t num_rings = (uint32_t)ring_count;
        memcpy(geom->coordinates, &num_rings, 4);
        unsigned char *dst = geom->coordinates + 4;

        for (size_t i = 0; i < ring_count; i++) {
            uint32_t num_points = (uint32_t)rings[i].count;
            memcpy(dst, &num_points, 4);
            dst += 4;
            for (size_t j = 0; j < rings[i].count; j++) {
                memcpy(dst, &rings[i].pts[j].x, 8);
                memcpy(dst + 8, &rings[i].pts[j].y, 8);
                dst += 16;
            }
            free(rings[i].pts);
        }
        free(rings);
        return 0;
    } else if (strncmp(wkt_str, "GEOMETRYCOLLECTION", 18) == 0) {
        geom->type = GEOM_TYPE_GEOMETRYCOLLECTION;
        const char *p = strchr(wkt_str, '(');
        if (!p) return -1;
        p++;

        size_t sub_cap = 4,
               sub_count = 0;
        SubGeom *sub_geoms = malloc(sub_cap * sizeof(SubGeom));

        // Analizar cada sub-geometría separada por comas dentro del contenedor
        while (*p && *p != ')') {
            while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
            if (*p == ')' || !*p) break;

            // Encontrar el inicio de la sub-geometría (ej. "POINT(...)", "LINESTRING(...)")
            const char *geom_start = p;
            int paren_depth = 0;
            while (*p) {
                if (*p == '(') paren_depth++;
                else if (*p == ')') {
                    paren_depth--;
                    if (paren_depth == 0) {
                        p++;
                        break;
                    }
                }
                p++;
            }

            size_t sub_wkt_len = p - geom_start;
            char *sub_wkt = malloc(sub_wkt_len + 1);
            memcpy(sub_wkt, geom_start, sub_wkt_len);
            sub_wkt[sub_wkt_len] = '\0';

            // Parsear recursivamente la sub-geometría
            MariaDBGeometry sub_g;
            if (parse_wkt_to_geometry(sub_wkt, &sub_g) != 0) {
                free(sub_wkt);
                for (size_t i = 0; i < sub_count; i++) free(sub_geoms[i].data);
                free(sub_geoms);
                return -1;
            }
            free(sub_wkt);

            // Construir el blob binario WKB interno de la sub-geometría:
            // 1 byte (Endianness) + 4 bytes (Tipo) + sub_g.coordinate_len (datos)
            size_t sub_binary_len = 1 + 4 + sub_g.coordinate_len;
            unsigned char *sub_binary = malloc(sub_binary_len);
            sub_binary[0] = sub_g.byte_order;
            uint32_t stype = (uint32_t)sub_g.type;
            memcpy(sub_binary + 1, &stype, 4);
            memcpy(sub_binary + 5, sub_g.coordinates, sub_g.coordinate_len);
            free(sub_g.coordinates);

            if (sub_count >= sub_cap) {
                sub_cap *= 2;
                sub_geoms = realloc(sub_geoms, sub_cap * sizeof(SubGeom));
            }
            sub_geoms[sub_count++] = (SubGeom){sub_binary, sub_binary_len};
        }

        // Calcular el tamaño total de la colección:
        // 4 bytes (num_geoms) + suma de los tamaños binarios de cada sub-geometría
        size_t total_len = 4;
        for (size_t i = 0; i < sub_count; i++)
            total_len += sub_geoms[i].len;

        geom->coordinate_len = total_len;
        geom->coordinates = malloc(total_len);
        if (!geom->coordinates) {
            for (size_t i = 0; i < sub_count; i++) free(sub_geoms[i].data);
            free(sub_geoms);
            return -1;
        }

        uint32_t num_geoms = (uint32_t)sub_count;
        memcpy(geom->coordinates, &num_geoms, 4);
        unsigned char *dst = geom->coordinates + 4;

        for (size_t i = 0; i < sub_count; i++) {
            memcpy(dst, sub_geoms[i].data, sub_geoms[i].len);
            dst += sub_geoms[i].len;
            free(sub_geoms[i].data);
        }
        free(sub_geoms);
        return 0;
    }
    // GEOM_TYPE_MULTIPOLYGON y GEOM_TYPE_GEOMETRYCOLLECTION se pueden estructurar de manera similar anidando niveles de conteo.
    
    geom->type = GEOM_TYPE_UNKNOWN;
    return -1;
}

int parse_internal_geometry(const unsigned char *src, size_t len, MariaDBGeometry *geom) {
    if (!src || len < 9 || !geom) return -1;

    /* 1. Extraer los 4 bytes del SRID */
    memcpy(&(geom->srid), src, 4);

    /* 2. Parsear el bloque WKB subsiguiente a partir del byte 4 */
    return parse_wkb_geometry(src + 4, len - 4, geom);
}

/* Parser para el formato WKB puro */
int parse_wkb_geometry(const unsigned char *src, size_t len, MariaDBGeometry *geom) {
    if (!src || len < 5 || !geom) return -1;

    /* 1. Byte de orden (1 = Little-Endian / NDR, 0 = Big-Endian / XDR) */
    geom->byte_order = src[0];

    /* 2. Tipo de geometría (4 bytes siguientes) */
    uint32_t raw_type;
    memcpy(&raw_type, src + 1, 4);
    geom->type = (GeometryType)(raw_type & 0xFFFF);

    /* 3. Coordenadas y datos restantes */
    geom->coordinate_len = len - 5;
    if (geom->coordinate_len > 0) {
        geom->coordinates = (unsigned char *)malloc(geom->coordinate_len);
        if (!geom->coordinates) return -1;
        memcpy(geom->coordinates, src + 5, geom->coordinate_len);
    } else {
        geom->coordinates = NULL;
    }

    return 0;
}

// 8. Wrappers expuestos a Lua ampliados
int wrap_sqlvalue_tiny(lua_State *L)     { l_sqlvalue_tiny(L); return 1; }
int wrap_sqlvalue_small(lua_State *L)    { l_sqlvalue_small(L); return 1; }
int wrap_sqlvalue_medium(lua_State *L)   { l_sqlvalue_medium(L); return 1; }
int wrap_sqlvalue_integer(lua_State *L)  { l_sqlvalue_integer(L); return 1; }
int wrap_sqlvalue_bigint(lua_State *L)   { l_sqlvalue_bigint(L); return 1; }
int wrap_sqlvalue_year(lua_State *L)     { l_sqlvalue_year(L); return 1; }
int wrap_sqlvalue_bit(lua_State *L)      { l_sqlvalue_bit(L); return 1; }
int wrap_sqlvalue_bool(lua_State *L)     { l_sqlvalue_bool(L); return 1; }

int wrap_sqlvalue_float(lua_State *L)    { l_sqlvalue_float(L); return 1; }
int wrap_sqlvalue_double(lua_State *L)   { l_sqlvalue_double(L); return 1; }
int wrap_sqlvalue_decimal(lua_State *L)  { l_sqlvalue_decimal(L); return 1; }

int wrap_sqlvalue_string(lua_State *L)   { l_sqlvalue_string(L); return 1; }
int wrap_sqlvalue_json(lua_State *L)     { l_sqlvalue_json(L); return 1; }
int wrap_sqlvalue_enum(lua_State *L)     { l_sqlvalue_enum(L); return 1; }
int wrap_sqlvalue_set(lua_State *L)      { l_sqlvalue_set(L); return 1; }

int wrap_sqlvalue_date(lua_State *L)     { l_sqlvalue_date(L); return 1; }
int wrap_sqlvalue_datetime(lua_State *L) { l_sqlvalue_datetime(L); return 1; }
int wrap_sqlvalue_time(lua_State *L)     { l_sqlvalue_time(L); return 1; }
int wrap_sqlvalue_timestamp(lua_State *L){ l_sqlvalue_timestamp(L); return 1; }

int wrap_sqlvalue_blob(lua_State *L)     { l_sqlvalue_blob(L); return 1; }
// Factoría para crear un SqlValue de geometría desde Lua (Soporta WKT y WKB binario)
int wrap_sqlvalue_geometry(lua_State *L) {
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);

    SqlValue *v = (SqlValue *)lua_newuserdata(L, sizeof(SqlValue));
    v->is_null = 0;
    v->sql_type = MYSQL_TYPE_GEOMETRY;

    // Guardamos los datos en crudo (sea WKT plano o binario)
    unsigned char *blob = (unsigned char *)malloc(len);
    if (!blob)
        return luaL_error(L, "Memoria insuficiente para geometry()");
    memcpy(blob, data, len);
    
    v->data.blob_val.ptr = blob;
    v->data.blob_val.len = len;

    // Asigna correctamente la metatabla geométrica y deja el userdata en la cima
    luaL_setmetatable(L, SQLVALUE_GEOM_META);
    
    return 1;
}