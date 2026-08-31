import("cmariadb", "chttp", "cjson")

print("[INFO] Inicializando servidor modular API UniversalWare. . .")

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

while true do
    local peticion = chttp.accept()
    if peticion then
        print("[PETICIÓN HTTP]: " .. tostring(peticion.method) .. " " .. tostring(peticion.path))

        local metodo = peticion.method:lower()
        if metodo == "get" and (peticion.path == "/" or peticion.path == "/index.html") then
            peticion:send_file(200, "web/public/index.html", "text/html; charset=utf-8")
        else
            local ruta_limpia = peticion.path:gsub("^/api", "")
            local recurso = ruta_limpia:match("^/([^/]+)") or ""
            local controller = controllers[recurso]
            if controller and controller[metodo] then
                local success, err = pcall(function()
                    local db = conectar_db()
                    local ok_ejecucion, resultado_o_error = pcall(function()
                        return controller[metodo](db, cjson, sanitizar, peticion)
                    end)
                    pcall(function() db:close() end)
                    if not ok_ejecucion then error(resultado_o_error) end
                end)

                if not success then
                    print("[ERROR 500]: " .. tostring(err))
                    pcall(function()
                        peticion:respond(500, cjson.encode({ error = "Error interno", detalle = tostring(err) }))
                    end)
                end
            else
                peticion:respond(404, cjson.encode({ error = "Recurso no encontrado" }))
            end
        end
    end
end