import("cmariadb", "system")

local db, err = cmariadb.connect({
    host = "127.0.0.1",
    user = "lua_client",
    password = "12345",
    db = "dark_kitchen_db",
    client_mode = cmariadb.CLIENT_MODE.MULTIPLE_STATEMENTS
})
if not db then
    error("[ERROR] No se pudo conectar a MariaDB: " .. tostring(err))
end
print("[INFO] Conexión establecida con MariaDB.")

-- ==========================================
-- CONSULTAS ANALÍTICAS DE LA DARK KITCHEN
-- ==========================================

print("\n--- 1. PRODUCTOS DE $30 A $70 ORDENADOS POR PRECIO ---")
local res1 = db:query([[
    SELECT id_producto, nombre, precio_venta 
    FROM productos 
    WHERE precio_venta BETWEEN 30.00 AND 70.00 
    ORDER BY precio_venta ASC;
]])
system.print(res1)
print("> Interpretación: Muestra el catálogo accesible ordenado de menor a mayor precio.")

print("\n--- 2. PRODUCTO Y CATEGORÍA CON INNER JOIN ---")
local res2 = db:query([[
    SELECT p.nombre AS producto, c.nombre AS categoria, p.precio_venta 
    FROM productos p
    INNER JOIN categorias c ON p.id_categoria = c.id_categoria;
]])
system.print(res2)
print("> Interpretación: Relaciona cada artículo con su respectiva clasificación culinaria.")

print("\n--- 3. TOTAL DE PRODUCTOS POR CATEGORÍA ---")
local res3 = db:query([[
    SELECT c.nombre AS categoria, COUNT(p.id_producto) AS total_productos
    FROM categorias c
    LEFT JOIN productos p ON c.id_categoria = p.id_categoria
    GROUP BY c.id_categoria, c.nombre;
]])
system.print(res3)
print("> Interpretación: Desglose cuantitativo del inventario por tipo de cocina.")

print("\n--- 4. CATEGORÍAS CON 2 O MÁS PRODUCTOS ---")
local res4 = db:query([[
    SELECT c.nombre AS categoria, COUNT(p.id_producto) AS total_productos
    FROM categorias c
    INNER JOIN productos p ON c.id_categoria = p.id_categoria
    GROUP BY c.id_categoria, c.nombre
    HAVING COUNT(p.id_producto) >= 2;
]])
system.print(res4)
print("> Interpretación: Filtra las líneas de negocio con mayor diversificación y solidez.")

print("\n--- 5. PRODUCTOS QUE NUNCA SE HAN VENDIDO ---")
local res5 = db:query([[
    SELECT p.id_producto, p.nombre AS producto_sin_ventas
    FROM productos p
    LEFT JOIN detalle_pedidos dp ON p.id_producto = dp.id_producto
    WHERE dp.id_pedido IS NULL;
]])
system.print(res5)
print("> Interpretación: Detecta artículos de bajo rendimiento comercial o 'muertos' sin historial de salida.")

db:close()