import("cmariadb", "system")

local db = assert(cmariadb.connect({
    host = "127.0.0.1",
    user = "lua_client",
    password = "12345",
    client_mode = cmariadb.CLIENT_MODE.MULTIPLE_STATEMENTS
}))

assert(db:multi_query([[
    DROP DATABASE IF EXISTS lab1;
    CREATE DATABASE IF NOT EXISTS lab1 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
    USE lab1;

    CREATE TABLE IF NOT EXISTS strs (
        col_enum ENUM('ok', 'error', 'running', 'yield'),
        col_set SET('a', 'b', 'c', 'd', 'e', 'f'),
        col_json JSON,
        col_blob BLOB
    );
]]))

assert(
    db:query(
        string.format(
        "INSERT INTO strs (col_enum, col_set, col_json, col_blob) "..
        "VALUES ('running', 'd,e,f', '{\"version\": \"%s\"}', LOAD_FILE('./2012-Lexus-LFA-Bring-A-Trailer-3.jpg'));",
        jit.version)
    )
)
local resultado = assert(db:query("SELECT * FROM strs;"))
system.print(resultado)

--[[
GRANT FILE ON *.* TO 'lua_client'@'127.0.0.1';
FLUSH PRIVILEGES;
]]