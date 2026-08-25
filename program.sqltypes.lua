import("cmariadb", "system")

local db, err = cmariadb.connect({
    host = "127.0.0.1",
    user = "lua_client",
    password = "12345",
    client_mode = cmariadb.CLIENT_MODE.MULTIPLE_STATEMENTS
})
if not db then
    error("[ERROR] No se pudo conectar a MariaDB: " .. tostring(err))
end

-- 1. Crear una tabla exhaustiva con absolutamente todos los tipos de datos ampliados
assert(db:query("DROP DATABASE IF EXISTS tipados_sql;"))
local create_table_query = [[
    CREATE DATABASE IF NOT EXISTS tipados_sql;
    USE tipados_sql;
    
    CREATE TABLE IF NOT EXISTS complete_datatype_demo (
        id INT AUTO_INCREMENT PRIMARY KEY,
        -- Numéricos enteros y pequeños
        col_tiny TINYINT,
        col_small SMALLINT,
        col_medium MEDIUMINT,
        col_int INT,
        col_bigint BIGINT,
        col_bit BIT(8),
        col_bool BOOLEAN,
        -- Decimales y flotantes
        col_float FLOAT,
        col_double DOUBLE,
        col_decimal DECIMAL(12,4),
        -- Fechas y tiempos
        col_year YEAR,
        col_date DATE,
        col_datetime DATETIME,
        col_time TIME,
        col_timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        -- Textos, JSON, ENUM, SET y Binarios
        col_string VARCHAR(150),
        col_json JSON,
        col_enum ENUM('activo', 'inactivo', 'pendiente'),
        col_set SET('leer', 'escribir', 'ejecutar'),
        col_blob LONGBLOB,
        col_geometry GEOMETRY
    );
]]

local res, err_q = db:multi_query(create_table_query)
if not res then
    error("[ERROR] No se pudo crear la tabla completa: " .. tostring(err_q))
end
print("[INFO] Tabla 'complete_datatype_demo' lista con todos los tipos ampliados.")

-- 2. Instanciar CADA tipo de dato usando la factoría ampliada de cmariadb.SqlValue
local SqlValue   = cmariadb.SqlValue
local v_tiny     = SqlValue.tinyint(1)
local v_small    = SqlValue.smallint(32000)
local v_medium   = SqlValue.mediumint(838860)
local v_int      = SqlValue.integer(2147483647)
local v_bigint   = SqlValue.bigint(9223372036854775807ULL or 9223372036854775807)
local v_bit      = SqlValue.bit(85) -- Ej: binario 01010101
local v_bool     = SqlValue.boolean(true)
local v_float    = SqlValue.float(123.45)
local v_double   = SqlValue.double(987654.321098)
local v_decimal  = SqlValue.decimal(55555.1234)
local v_year     = SqlValue.year(2026)
local v_date     = SqlValue.date("2026-08-24")
local v_datetime = SqlValue.datetime("2026-08-24 18:00:00")
local v_time     = SqlValue.time("18:00:00")
local v_timestamp= SqlValue.timestamp("2026-08-24 18:00:00")
local v_string   = SqlValue.string("Registro maestro con espectro completo de tipos")
local v_json     = SqlValue.json('{"modulo": "cmariadb", "estado": "ok"}')
local v_enum     = SqlValue.enum("activo")
local v_set      = SqlValue.set("leer,escribir")
local v_blob     = SqlValue.blob("Datos binarios simulados o serializados en buffer")
local v_geometry = SqlValue.geometry("POINT(19.4326 -99.1332)")

system.print("v_tiny: ", v_tiny)
system.print("v_small: ", v_small)
system.print("v_medium: ", v_medium)
system.print("v_int: ", v_int)
system.print("v_bigint: ", v_bigint)
system.print("v_bit: ", v_bit)
system.print("v_bool: ", v_bool)
system.print("v_float: ", v_float)
system.print("v_double: ", v_double)
system.print("v_decimal: ", v_decimal)
system.print("v_year: ", v_year)
system.print("v_date: ", v_date)
system.print("v_datetime: ", v_datetime)
system.print("v_time: ", v_time)
system.print("v_timestamp: ", v_timestamp)
system.print("v_string: ", v_string)
system.print("v_json: ", v_json)
system.print("v_enum: ", v_enum)
system.print("v_set: ", v_set)
system.print("v_blob: ", v_blob)
system.print("v_geometry: ", v_geometry)

-- 3. Construir la sentencia de inserción usando ST_GeomFromText para el tipo GEOMETRY
local insert_query = string.format(
    [[INSERT INTO complete_datatype_demo 
      (col_tiny, col_small, col_medium, col_int, col_bigint, col_bit, col_bool, col_float, col_double, col_decimal, col_year, col_date, col_datetime, col_time, col_timestamp, col_string, col_json, col_enum, col_set, col_blob, col_geometry) 
      VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', ST_GeomFromText('%s'));]],
    tostring(v_tiny), tostring(v_small), tostring(v_medium), tostring(v_int), tostring(v_bigint),
    tostring(v_bit), tostring(v_bool), tostring(v_float), tostring(v_double), tostring(v_decimal), tostring(v_year),
    tostring(v_date), tostring(v_datetime), tostring(v_time), tostring(v_timestamp),
    tostring(v_string), tostring(v_json), tostring(v_enum), tostring(v_set),
    tostring(v_blob), tostring(v_geometry)
)

local insert_res, err_ins = db:query(insert_query)
if not insert_res then
    error("[ERROR] Falló la inserción completa: " .. tostring(err_ins))
end
print("[INFO] ¡Fila insertada exitosamente cubriendo todos los tipos nativos y extendidos de MariaDB!")

-- 4. Consultar y verificar el resultado
local rows = db:query("SELECT id, col_tiny, col_bigint, col_decimal, col_datetime, col_json, col_enum, col_set, col_blob FROM complete_datatype_demo;")
if rows and #rows > 0 then
    local r = rows[1]
    print("\n--- VERIFICACIÓN DE TIPOS RECUPERADOS ---")
    print("TinyInt:   ", tostring(r.col_tiny))
    print("BigInt:    ", tostring(r.col_bigint))
    print("Decimal:   ", tostring(r.col_decimal))
    print("DateTime:  ", tostring(r.col_datetime))
    print("JSON:      ", tostring(r.col_json))
    print("Enum:      ", tostring(r.col_enum))
    print("Set:       ", tostring(r.col_set))
    print("Blob/Text: ", tostring(r.col_blob))
end

-- 5. Comprobación de tipos nativos reales en el esquema de MariaDB
local schema_check_query = [[
    SELECT COLUMN_NAME, DATA_TYPE, COLUMN_TYPE, CHARACTER_MAXIMUM_LENGTH
    FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE() 
      AND TABLE_NAME = 'complete_datatype_demo';
]]

local schema_rows = db:query(schema_check_query)
if schema_rows and #schema_rows > 0 then
    print("\n--- TIPOS NATIVOS REGISTRADOS EN EL ESQUEMA (INFORMATION_SCHEMA) ---")
    for _, col in ipairs(schema_rows) do
        print(string.format("Columna: %-15s | Tipo SQL: %-12s | Definición: %-20s", 
            col.COLUMN_NAME, col.DATA_TYPE, col.COLUMN_TYPE))
    end
end

db:close()