import("cmariadb", "cjob", "chttp", "json")

print("[INFO] Inicializando servidor modular API UniversalWare...")

local function conectar_db()
    local db, err = cmariadb.connect({
        host = "127.0.0.1",
        user = "lua_client",
        password = "12345",
        db = "dark_kitchen_db"
    })
    if not db then error("Error conectando a MariaDB: " .. tostring(err)) end
    return db
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

local controllers = {
    productos  = require("fetch.productos"),
    clientes   = require("fetch.clientes"),
    categorias = require("fetch.categorias")
}

cjob.new(function()
    chttp.listen("127.0.0.1", 8081)
    print("[INFO] Servidor HTTP escuchando y listo en el puerto 8081.")

    while true do
        local peticion = chttp.accept()
        if peticion then
            local recurso = peticion.path:match("^/([^/]+)") or ""
            local metodo = peticion.method:lower()

            print("[PETICIÓN HTTP]: " .. peticion.method .. " " .. peticion.path)

            local controller = controllers[recurso]
            if controller and controller[metodo] then
                local success, err = pcall(function()
                    local db = conectar_db()
                    -- Garantizamos que db:close() se llame pase lo que pase usando un xpcall o pcall interno
                    local ok_ejecucion, resultado_o_error = pcall(function()
                        return controller[metodo](db, json, sanitizar, peticion)
                    end)
                    
                    -- Cerramos la BD inmediatamente después de terminar (exito o fallo)
                    pcall(function() db:close() end)
                    
                    if not ok_ejecucion then
                        error(resultado_o_error)
                    end
                end)

                if not success then
                    print("[ERROR 500 Intermitente]: " .. tostring(err))
                    pcall(function()
                        peticion:respond(500, json.encode({ 
                            error = "Error interno del servidor", 
                            detalle = tostring(err) 
                        }))
                    end)
                end
            else
                peticion:respond(404, json.encode({ 
                    error = "Endpoint no encontrado o método no soportado", 
                    path = peticion.path 
                }))
            end
        else
            -- Si accept() regresa nil, cedemos el control brevemente a la corrutina 
            -- para evitar un bucle infinito que queme la CPU y sature la terminal.
            cjob.wait(0.01) -- O usa una pausa equivalente soportada por tu entorno cjob
        end
    end
end)

cjob.async()