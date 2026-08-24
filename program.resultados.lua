import("cmariadb", "system")

-- 1. Conexión a la base de datos existente
local db, err = cmariadb.connect("localhost", "root", "12345", "dark_kitchen_db", 3306)
if not db then
    error("[ERROR] No se pudo conectar a MariaDB: " .. tostring(err))
end
print("==================================================")
print("   DARK KITCHEN - SISTEMA DE REPORTE ANALÍTICO    ")
print("==================================================\n")

-- Helper para formatear encabezados de reportes
local function imprimir_encabezado(titulo)
    print("\n--------------------------------------------------")
    print(" " .. string.upper(titulo))
    print("--------------------------------------------------")
end

-- REPORTE 1: Ventas e ingresos totales por Plataforma
imprimir_encabezado("1. Rendimiento de Ventas por Plataforma")
local q1 = [[
    SELECT 
        plataforma_origen,
        COUNT(id_pedido) AS total_pedidos,
        SUM(total) AS ingresos_totales,
        ROUND(AVG(total), 2) AS ticket_promedio
    FROM pedidos
    WHERE estado != 'Cancelado'
    GROUP BY plataforma_origen
    ORDER BY ingresos_totales DESC;
]]
local res1, err1 = db:query(q1)
if res1 then
    system.print(res1)
else
    print("[ERROR] Consulta 1 falló: " .. tostring(err1))
end

-- REPORTE 2: Top Productos Más Vendidos
imprimir_encabezado("2. Top 5 Productos Más Vendidos")
local q2 = [[
    SELECT 
        p.nombre AS producto,
        m.nombre AS marca,
        SUM(dp.cantidad) AS unidades_vendidas,
        SUM(dp.cantidad * dp.precio_unitario) AS ingresos_generados
    FROM detalle_pedidos dp
    JOIN productos p ON dp.id_producto = p.id_producto
    JOIN marcas m ON p.id_marca = m.id_marca
    GROUP BY p.id_producto, p.nombre, m.nombre
    ORDER BY unidades_vendidas DESC
    LIMIT 5;
]]
local res2, err2 = db:query(q2)
if res2 then
    system.print(res2)
else
    print("[ERROR] Consulta 2 falló: " .. tostring(err2))
end

-- REPORTE 3: Alerta de Reabastecimiento de Stock
imprimir_encabezado("3. Alerta de Insumos Críticos (Stock <= Stock Mínimo)")
local q3 = [[
    SELECT 
        id_ingrediente,
        nombre AS ingrediente,
        stock_actual,
        stock_minimo,
        unidad_medida,
        CASE 
            WHEN stock_actual = 0 THEN 'CRÍTICO: SIN STOCK'
            WHEN stock_actual <= stock_minimo THEN 'REORDENAR'
            ELSE 'OK'
        END AS estado_inventario
    FROM ingredientes
    WHERE stock_actual <= stock_minimo
    ORDER BY stock_actual ASC;
]]
local res3, err3 = db:query(q3)
if res3 then
    if #res3 == 0 then
        print("[OK] Todos los insumos cuentan con stock suficiente por encima del mínimo.")
    else
        system.print(res3)
    end
else
    print("[ERROR] Consulta 3 falló: " .. tostring(err3))
end

-- REPORTE 4: Eficiencia de Repartidores
imprimir_encabezado("4. Resumen de Entregas por Repartidor")
local q4 = [[
    SELECT 
        r.nombre AS repartidor,
        r.vehiculo,
        COUNT(p.id_pedido) AS pedidos_asignados,
        SUM(CASE WHEN p.estado = 'Entregado' THEN 1 ELSE 0 END) AS pedidos_entregados
    FROM repartidores r
    LEFT JOIN pedidos p ON r.id_repartidor = p.id_repartidor
    GROUP BY r.id_repartidor, r.nombre, r.vehiculo
    ORDER BY pedidos_entregados DESC;
]]
local res4, err4 = db:query(q4)
if res4 then
    system.print(res4)
else
    print("[ERROR] Consulta 4 falló: " .. tostring(err4))
end

print("\n==================================================")
print("         FIN DEL REPORTE DE OPERACIONES           ")
print("==================================================")