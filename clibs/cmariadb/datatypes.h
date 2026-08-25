#ifndef CMARIADB_DATATYPES_H
    #define CMARIADB_DATATYPES_H

    #include <lua.h>
    #include <lauxlib.h>
    #include <mysql/mysql.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdint.h>
    #include <stddef.h>
    #include <stdio.h>
    #include <stdint.h>
    #include <ctype.h>
    #include <arpa/inet.h>

    // #define SQLVALUE_META "SqlValueMeta"
    #define MARIADB_LUA_METATABLE "MariaDB.Connection"
    #define MARIADB_SQLVALUE_METATABLE "cmariadb.SqlValue.meta"
    #define SQLVALUE_META "MariaDB.SqlValue"
    #define SQLVALUE_GEOM_META "MariaDB.SqlValue.Geometry"

    // Estructura contenedora para el Userdata de Lua (Conexión)
    typedef struct {
        MYSQL *conn;
    } LuaMariaDB;

    // Estructura ampliada para soportar todos los tipos de MariaDB/MySQL
    typedef struct {
        enum enum_field_types sql_type; // Identificador exacto de MariaDB (MYSQL_TYPE_*)
        
        // Unión optimizada para albergar el valor nativo según corresponda
        union {
            int64_t int_val;            // TINYINT, SMALLINT, INT, BIGINT, YEAR, BIT, BOOLEAN
            double float_val;           // FLOAT, DOUBLE, DECIMAL, NEWDECIMAL
            struct {
                char *ptr;              // Puntero dinámico para VARCHAR, TEXT, JSON, ENUM, SET, DATE, DATETIME, TIME
                size_t len;             // Longitud exacta de los datos (indispensable para binarios y BLOBs)
            } string_val;
            struct {
                const unsigned char *ptr; // Para datos espaciales (GEOMETRY) o BLOBs crudos
                size_t len;
            } blob_val;
        } data;
        
        int is_null;                    // Bandera estricta para valores NULL de SQL
    } SqlValue;

    // Funciones base de creación e implementación
    SqlValue* l_sqlvalue_create_impl(lua_State *L, enum enum_field_types type, int is_int);
    SqlValue* new_sql_value(lua_State *L, enum enum_field_types type);

    // Estructura auxiliar interna para construir listas dinámicas de puntos durante el parseo WKT
    typedef struct {
        double x, y;
    } WKTPoint;

    typedef struct {
        WKTPoint *points;
        size_t count;
        size_t capacity;
    } WKTPointList;

    // Estructura auxiliar temporal para almacenar sub-geometrías parseadas
    typedef struct {
        unsigned char *data;
        size_t len;
    } SubGeom;

    /* Tipos de geometrías soportados según OpenGIS */
    typedef enum {
        GEOM_TYPE_UNKNOWN = 0,
        GEOM_TYPE_POINT = 1,
        GEOM_TYPE_LINESTRING = 2,
        GEOM_TYPE_POLYGON = 3,
        GEOM_TYPE_MULTIPOINT = 4,
        GEOM_TYPE_MULTILINESTRING = 5,
        GEOM_TYPE_MULTIPOLYGON = 6,
        GEOM_TYPE_GEOMETRYCOLLECTION = 7
    } GeometryType;

    /* Estructura alineada al almacenamiento interno de MariaDB/MySQL */
    typedef struct {
        uint32_t srid;            /* 4 bytes: Spatial Reference System Identifier */
        uint8_t byte_order;       /* 1 byte: 1 = Little-Endian (NDR), 0 = Big-Endian (XDR) */
        GeometryType type;        /* 4 bytes: Código de tipo de geometría (1-7) */
        size_t coordinate_len;    /* Longitud del payload de coordenadas */
        unsigned char *coordinates; /* Puntero a los bytes de coordenadas (ej. dobles IEEE 754) */
    } MariaDBGeometry;

    /* Funciones para manejo de GEOMETRY */
    int parse_wkt_to_geometry(const char *wkt_str, MariaDBGeometry *geom);
    int serialize_geometry_to_wkb(const MariaDBGeometry *geom, unsigned char *buf, size_t buf_len);
    int parse_internal_geometry(const unsigned char *src, size_t len, MariaDBGeometry *geom);
    int parse_wkb_geometry(const unsigned char *src, size_t len, MariaDBGeometry *geom);

    // --- MACROS CONSTRUCTORAS AMPLIADAS ---
    // Numéricos
    #define l_sqlvalue_tiny(L)      (l_sqlvalue_create_impl((L), MYSQL_TYPE_TINY, 1))
    #define l_sqlvalue_small(L)     (l_sqlvalue_create_impl((L), MYSQL_TYPE_SHORT, 1))
    #define l_sqlvalue_medium(L)    (l_sqlvalue_create_impl((L), MYSQL_TYPE_INT24, 1))
    #define l_sqlvalue_integer(L)   (l_sqlvalue_create_impl((L), MYSQL_TYPE_LONG, 1))
    #define l_sqlvalue_bigint(L)    (l_sqlvalue_create_impl((L), MYSQL_TYPE_LONGLONG, 1))
    #define l_sqlvalue_year(L)      (l_sqlvalue_create_impl((L), MYSQL_TYPE_YEAR, 1))
    #define l_sqlvalue_bit(L)       (l_sqlvalue_create_impl((L), MYSQL_TYPE_BIT, 1))
    #define l_sqlvalue_bool(L)      (l_sqlvalue_create_impl((L), MYSQL_TYPE_BOOL, 1))

    // Decimales y Flotantes
    #define l_sqlvalue_float(L)     (l_sqlvalue_create_impl((L), MYSQL_TYPE_FLOAT, 0))
    #define l_sqlvalue_double(L)    (l_sqlvalue_create_impl((L), MYSQL_TYPE_DOUBLE, 0))
    #define l_sqlvalue_decimal(L)   (l_sqlvalue_create_impl((L), MYSQL_TYPE_NEWDECIMAL, 0))

    // Cadenas, Textos y JSON
    #define l_sqlvalue_string(L)    (l_sqlvalue_create_impl((L), MYSQL_TYPE_VAR_STRING, 0))
    #define l_sqlvalue_json(L)      (l_sqlvalue_create_impl((L), MYSQL_TYPE_JSON, 0))
    #define l_sqlvalue_enum(L)      (l_sqlvalue_create_impl((L), MYSQL_TYPE_ENUM, 0))
    #define l_sqlvalue_set(L)       (l_sqlvalue_create_impl((L), MYSQL_TYPE_SET, 0))

    // Fechas y Horas
    #define l_sqlvalue_date(L)      (l_sqlvalue_create_impl((L), MYSQL_TYPE_DATE, 0))
    #define l_sqlvalue_datetime(L)  (l_sqlvalue_create_impl((L), MYSQL_TYPE_DATETIME, 0))
    #define l_sqlvalue_time(L)      (l_sqlvalue_create_impl((L), MYSQL_TYPE_TIME, 0))
    #define l_sqlvalue_timestamp(L) (l_sqlvalue_create_impl((L), MYSQL_TYPE_TIMESTAMP, 0))

    // Binarios, Blobs y Geometría
    #define l_sqlvalue_blob(L)      (l_sqlvalue_create_impl((L), MYSQL_TYPE_LONG_BLOB, 0))
    // #define l_sqlvalue_geometry(L)  (l_sqlvalue_create_impl((L), MYSQL_TYPE_GEOMETRY, 0))

    // Registro y Deserialización
    void register_sqlvalue_meta(lua_State *L);
    void push_mariadb_field(lua_State *L, MYSQL_FIELD *field, char *val, unsigned long length);

    // --- WRAPPERS EXPUESTOS HACIA LUA ---
    int wrap_sqlvalue_tiny(lua_State *L);
    int wrap_sqlvalue_small(lua_State *L);
    int wrap_sqlvalue_medium(lua_State *L);
    int wrap_sqlvalue_integer(lua_State *L);
    int wrap_sqlvalue_bigint(lua_State *L);
    int wrap_sqlvalue_year(lua_State *L);
    int wrap_sqlvalue_bit(lua_State *L);
    int wrap_sqlvalue_bool(lua_State *L);

    int wrap_sqlvalue_float(lua_State *L);
    int wrap_sqlvalue_double(lua_State *L);
    int wrap_sqlvalue_decimal(lua_State *L);

    int wrap_sqlvalue_string(lua_State *L);
    int wrap_sqlvalue_json(lua_State *L);
    int wrap_sqlvalue_enum(lua_State *L);
    int wrap_sqlvalue_set(lua_State *L);

    int wrap_sqlvalue_date(lua_State *L);
    int wrap_sqlvalue_datetime(lua_State *L);
    int wrap_sqlvalue_time(lua_State *L);
    int wrap_sqlvalue_timestamp(lua_State *L);

    int wrap_sqlvalue_blob(lua_State *L);
    int wrap_sqlvalue_geometry(lua_State *L);
#endif