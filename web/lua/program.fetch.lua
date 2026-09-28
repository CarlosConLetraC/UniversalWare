import("cmariadb", "chttp", "cjson", "cjob", "ffi")

print("[INFO] Inicializando servidor modular API UniversalWare. . .")
ffi.cdef[[
    struct timeval {
        long tv_sec;
        long tv_usec;
    };
    int gettimeofday(struct timeval *tv, void *tz);
]]
local tv = ffi.new("struct timeval")

local function get_exact_timestamp()
    local tv = ffi.new("struct timeval")
    ffi.C.gettimeofday(tv, nil)
    local sec = tonumber(tv.tv_sec)
    local ms = math.floor(tonumber(tv.tv_usec) / 1000)
    
    local date_part = os.date("%a %b %d %H:%M:%S", sec)
    local year_part = os.date("%Y", sec)
    
    return string.format("[%s:%03ds %s]", date_part, ms, year_part)
end

local db_global
local firmas_db = {
    host = "127.0.0.1",
    user = "lua_client",
    password = "12345",
    db = "dark_kitchen_db"
}

local function obtener_conexion_db()
    if not db_global then
        local ok, db_or_err = pcall(cmariadb.connect, firmas_db)
        if ok and db_or_err then
            db_global = db_or_err
            print("[DATABASE] Nueva conexión persistente establecida con MariaDB.")
        else
            error("Error al crear conexión persistente: " .. tostring(db_or_err))
        end
    else
        -- Si ya existe, ejecutamos una consulta ligera ("ping") para verificar si sigue viva. . .
        local vivo, _ = pcall(function() return db_global:query("SELECT 1;") end)
        if not vivo then
            print("[DATABASE] Conexión caída o inactiva (Timeout). Intentando reconexión...")
            -- Limpieza preventiva del objeto antiguo para llamar a su destructor (__gc). . .
            pcall(function() db_global:close() end)
            db_global = nil
            
            -- Reintento inmediato de enlace
            local ok, res = pcall(cmariadb.connect, firmas_db)
            if ok then
                db_global = assert(res, "fetch panic: no se retornó valor alguno en interfaz luajit.")
                print("[DATABASE] Reconexión exitosa. Sesión restaurada.")
            else
                error("Fallo crítico en reconexión a MariaDB: " .. res)
            end
        end
    end
    return db_global
end

local function sanitizar(obj)
    local t = type(obj)
    if t == "table" then
        local nueva = {}
        for k, v in pairs(obj) do nueva[k] = sanitizar(v) end
        return nueva
    elseif t == "userdata" then
        if type(obj.value) == "function" then return sanitizar(obj:value()) end
        return tostring(obj)
    end
    return obj
end

local function escape_sql(str)
    return type(str) ~= "string" ? str : str:gsub("\\", "\\\\"):gsub("'", "\\'"):gsub('"', '\\"'):gsub("%z", "\\0")
end

local controllers = {
    marcas           = require("fetch.marcas"),
    categorias       = require("fetch.categorias"),
    productos        = require("fetch.productos"),
    ingredientes     = require("fetch.ingredientes"),
    recetas          = require("fetch.recetas"),
    clientes         = require("fetch.clientes"),
    repartidores     = require("fetch.repartidores"),
    pedidos          = require("fetch.pedidos"),
    detalle_pedidos  = require("fetch.detalle_pedidos")
}

chttp.listen("127.0.0.1", 8081)
print("[INFO] Servidor HTTP escuchando y listo en el puerto 8081.")

local NOMBRE_DE_TABLAS_DB = {}
do
    local db = obtener_conexion_db()
    local resultado = assert(db:query("SHOW TABLES;"))
    for _, v in pairs(resultado) do
        table.insert(NOMBRE_DE_TABLAS_DB, v.Tables_in_dark_kitchen_db:value())
    end
end

cjob.new(function()
    while true do
        local peticion = chttp.accept()
        if peticion then
            print(string.format("%s [PETICIÓN HTTP]: %s %s", 
                get_exact_timestamp(), 
                tostring(peticion.method), 
                tostring(peticion.path)
            ))

            local metodo = peticion.method:lower()
            
            -- Rutas de archivos estáticos
            if metodo == "get" and (peticion.path == "/" or peticion.path == "/index.html") then
                peticion:send_file(200, "web/public/index.html", "text/html; charset=utf-8")
            elseif metodo == "get" and peticion.path == "/script.js" then
                peticion:send_file(200, "web/public/script.js", "application/javascript; charset=utf-8")
            elseif metodo == "get" and peticion.path == "/styles.css" then
                peticion:send_file(200, "web/public/styles.css", "text/css; charset=utf-8")
            else
                -- Enrutamiento dinámico de Endpoints de API (/api/<recurso>)
                local ruta_limpia = peticion.path:gsub("^/api", "")
                local recurso = ruta_limpia:match("^/([^%?]+)") or ""
                local controller = controllers[recurso]

                if controller and controller[metodo] then
                    local success, err = pcall(function()
                        local db = obtener_conexion_db()
                        return controller[metodo](db, cjson, sanitizar, peticion, escape_sql)
                    end)

                    -- Manejo centralizado e Inteligente de Errores HTTP. . .
                    if not success then
                        local err_str = tostring(err)
                        local status_code = 500
                        local tipo_error = "Error interno del servidor"

                        -- Detección automática y refinada de dependencias en llaves foráneas. . .
                        if err_str:find("1451") or err_str:find("foreign key constraint fails") then
                            status_code = 409
                            tipo_error = "Conflicto de integridad referencial"
                            
                            -- MariaDB siempre coloca la tabla hija bloqueadora justo después de la base de datos: `db`.`tabla_hija`. . .
                            local tabla_bloqueo = err_str:match("`[^`]+`%.`(%w+)`")
                            
                            -- Extraer la columna de la llave foránea involucrada. . .
                            local columna_fk = err_str:match("FOREIGN KEY%s*%(`([^`]+)`%)")

                            -- Respaldo por si el formato varía: buscar con :find en la lista de tablas del sistema. . .
                            if not tabla_bloqueo then
                                local err_lower = err_str:lower()
                                for _, s in pairs(NOMBRE_DE_TABLAS_DB) do
                                    if err_lower:find("`"..s.."`", 1, true) then
                                        tabla_bloqueo = s
                                        break
                                    end
                                end
                            end

                            if tabla_bloqueo and columna_fk then
                                err_str = string.format("No se puede eliminar el registro porque la columna '%s' tiene dependencias activas en la tabla '%s'.", columna_fk, tabla_bloqueo)
                            elseif tabla_bloqueo then
                                err_str = string.format("No se puede eliminar el registro debido a dependencias activas en la tabla '%s'.", tabla_bloqueo)
                            else
                                err_str = "No se puede eliminar el registro porque viola restricciones de clave foránea en el sistema."
                            end
                        elseif err_str:find("inválido") or err_str:find("faltante") or err_str:lower():find("parámetro") then
                            status_code = 400
                            tipo_error = "Solicitud incorrecta"
                        end

                        print(string.format("%s [ERROR %d]: %s %s", 
                            get_exact_timestamp(),
                            status_code,
                            tipo_error,
                            err_str
                        ))
                        pcall(function()
                            peticion:respond(status_code, cjson.encode({ 
                                error = tipo_error, 
                                detalle = err_str 
                            }))
                        end)
                    end
                else
                    peticion:respond(404, cjson.encode({ error = "Recurso or método no encontrado" }))
                end
            end
        end
    end
end)

cjob.async()