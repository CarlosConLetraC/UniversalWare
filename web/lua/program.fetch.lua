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
            -- Limpieza preventiva del objeto antiguo para llamar a su destructor (__gc)
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
                        -- Obtenemos la conexión persistente (reutiliza o reconecta automáticamente). . .
                        local db = obtener_conexion_db()
                        
                        -- Ejecución segura de la lógica CRUD en tu controlador externo. . .
                        local ok_ejecucion, resultado_o_error = pcall(function()
                            return controller[metodo](db, cjson, sanitizar, peticion, escape_sql)
                        end)
                        
                        if not ok_ejecucion then error(resultado_o_error) end
                    end)

                    if not success then
                        print("[ERROR 500]: " .. tostring(err))
                        pcall(function()
                            peticion:respond(500, cjson.encode({ error = "Error interno", detalle = tostring(err) }))
                        end)
                    end
                else
                    peticion:respond(404, cjson.encode({ error = "Recurso o método no encontrado" }))
                end
            end
        end
    end
end)

cjob.async()
