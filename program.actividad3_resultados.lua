import("cmariadb", "system")

-- 1. Conexión a MariaDB
local db, err = cmariadb.connect({
    host = "127.0.0.1",
    user = "lua_client",
    password = "12345",
    db = "autos_concesionario_db",
    client_mode = cmariadb.CLIENT_MODE.MULTIPLE_STATEMENTS
})

if not db then
    error("[ERROR] No se pudo conectar a la base de datos: " .. tostring(err))
end

print("==================================================")
print("     MUESTRA DE DATOS: autos_concesionario_db     ")
print("==================================================\n")

local function sanitizar_tabla(t)
    for k, v in pairs(t) do
        local isSqlValue, mt
        if type(v) == "userdata" then
            mt = debug.getmetatable(v)
            isSqlValue = mt ? mt : false
            t[k] = (isSqlValue and type(v) == "userdata") and v:value() or v
        elseif type(v) == "table" then
            t[k] = sanitizar_tabla(v)
        end
    end
    return t
end

-- 2. Función auxiliar para ejecutar e imprimir consultas
local function mostrar_tabla(titulo, sql)
    print(">>> " .. titulo)
    local res, q_err = db:query(sql)
    if not res then
        print("[ERROR EN CONSULTA]: " .. tostring(q_err))
    else
        res = sanitizar_tabla(res)
        system.print(res)
    end
    print("\n" .. string.rep("-", 50) .. "\n")
end

-- 3. Consultas de muestra
mostrar_tabla("CATÁLOGO DE MODELOS (LIMIT 5)", [[
    SELECT id_modelo, marca, modelo, anho, precio_base, tipo_motor 
    FROM modelos_autos 
    LIMIT 5;
]])

mostrar_tabla("STOCK DISPONIBLE POR CONCESIONARIO (JOIN, LIMIT 5)", [[
    SELECT a.numero_serie, m.marca, m.modelo, c.nombre AS concesionario, a.ubicacion
    FROM automoviles_stock a
    JOIN modelos_autos m ON a.id_modelo = m.id_modelo
    JOIN concesionarios c ON a.id_concesionario = c.id_concesionario
    LIMIT 5;
]])

mostrar_tabla("RESUMEN DE VENTAS Y VENDEDORES (JOIN, LIMIT 5)", [[
    SELECT v.id_venta, v.numero_serie, vend.nombre AS vendedor, v.precio_venta, v.modo_pago, v.fecha_entrega
    FROM ventas v
    JOIN vendedores vend ON v.id_vendedor = vend.id_vendedor
    LIMIT 5;
]])

mostrar_tabla("TOTAL DE VENTAS ACUMULADAS REGISTRADAS", [[
    SELECT COUNT(*) AS total_registros, SUM(total) AS monto_total_acumulado
    FROM ventas_acumuladas;
]])

db:close()
print("[INFO] Inspección de muestra completada.")