import("cmariadb", "system")

-- 1. Conexión a MariaDB activando MULTIPLE_STATEMENTS para scripts DDL
local db, err = cmariadb.connect({
    host = "127.0.0.1",
    user = "lua_client",
    password = "12345",
    client_mode = cmariadb.CLIENT_MODE.MULTIPLE_STATEMENTS
})
if not db then
    error("[ERROR] No se pudo conectar a MariaDB: " .. tostring(err))
end
print("[INFO] Conexión establecida con MariaDB.")

-- 2. Creación y selección de la base de datos
assert(db:query("DROP DATABASE IF EXISTS dark_kitchen_db;"))
local ok, err = db:query("CREATE DATABASE IF NOT EXISTS dark_kitchen_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;")
if not ok then
    error("[ERROR] Al crear la base de datos: " .. tostring(err))
end

ok, err = db:query("USE dark_kitchen_db;")
if not ok then
    error("[ERROR] Al seleccionar la base de datos: " .. tostring(err))
end
print("[INFO] Base de datos 'dark_kitchen_db' seleccionada.")

-- 3. Definición del Esquema DDL (Tablas y Llaves Foráneas)
local ddl_schema = [[
    CREATE TABLE IF NOT EXISTS marcas (
        id_marca INT AUTO_INCREMENT PRIMARY KEY,
        nombre VARCHAR(100) NOT NULL UNIQUE,
        activa BOOLEAN DEFAULT TRUE
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS categorias (
        id_categoria INT AUTO_INCREMENT PRIMARY KEY,
        nombre VARCHAR(80) NOT NULL UNIQUE
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS productos (
        id_producto INT AUTO_INCREMENT PRIMARY KEY,
        id_marca INT NOT NULL,
        id_categoria INT NOT NULL,
        nombre VARCHAR(120) NOT NULL,
        precio_venta DECIMAL(10,2) NOT NULL CHECK (precio_venta > 0),
        FOREIGN KEY (id_marca) REFERENCES marcas(id_marca) ON DELETE RESTRICT ON UPDATE CASCADE,
        FOREIGN KEY (id_categoria) REFERENCES categorias(id_categoria) ON DELETE RESTRICT ON UPDATE CASCADE
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS ingredientes (
        id_ingrediente INT AUTO_INCREMENT PRIMARY KEY,
        nombre VARCHAR(100) NOT NULL UNIQUE,
        unidad_medida VARCHAR(20) NOT NULL,
        stock_actual DECIMAL(10,3) NOT NULL DEFAULT 0.000 CHECK (stock_actual >= 0),
        stock_minimo DECIMAL(10,3) NOT NULL DEFAULT 1.000
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS recetas (
        id_producto INT NOT NULL,
        id_ingrediente INT NOT NULL,
        cantidad_requerida DECIMAL(10,3) NOT NULL CHECK (cantidad_requerida > 0),
        PRIMARY KEY (id_producto, id_ingrediente),
        FOREIGN KEY (id_producto) REFERENCES productos(id_producto) ON DELETE CASCADE ON UPDATE CASCADE,
        FOREIGN KEY (id_ingrediente) REFERENCES ingredientes(id_ingrediente) ON DELETE RESTRICT ON UPDATE CASCADE
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS clientes (
        id_cliente INT AUTO_INCREMENT PRIMARY KEY,
        nombre VARCHAR(100) NOT NULL,
        telefono VARCHAR(20) NOT NULL,
        direccion VARCHAR(255) NOT NULL
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS repartidores (
        id_repartidor INT AUTO_INCREMENT PRIMARY KEY,
        nombre VARCHAR(100) NOT NULL,
        telefono VARCHAR(20) NOT NULL,
        vehiculo VARCHAR(50) NOT NULL
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS pedidos (
        id_pedido INT AUTO_INCREMENT PRIMARY KEY,
        id_cliente INT NOT NULL,
        id_repartidor INT NOT NULL,
        plataforma_origen ENUM('UberEats', 'Rappi', 'DidiFood', 'WebPropia') NOT NULL,
        fecha_hora DATETIME DEFAULT CURRENT_TIMESTAMP,
        estado ENUM('Pendiente', 'En Preparacion', 'En Camino', 'Entregado', 'Cancelado') DEFAULT 'Pendiente',
        total DECIMAL(10,2) NOT NULL CHECK (total >= 0),
        FOREIGN KEY (id_cliente) REFERENCES clientes(id_cliente) ON DELETE RESTRICT ON UPDATE CASCADE,
        FOREIGN KEY (id_repartidor) REFERENCES repartidores(id_repartidor) ON DELETE RESTRICT ON UPDATE CASCADE
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS detalle_pedidos (
        id_pedido INT NOT NULL,
        id_producto INT NOT NULL,
        cantidad INT NOT NULL CHECK (cantidad > 0),
        precio_unitario DECIMAL(10,2) NOT NULL CHECK (precio_unitario > 0),
        PRIMARY KEY (id_pedido, id_producto),
        FOREIGN KEY (id_pedido) REFERENCES pedidos(id_pedido) ON DELETE CASCADE ON UPDATE CASCADE,
        FOREIGN KEY (id_producto) REFERENCES productos(id_producto) ON DELETE RESTRICT ON UPDATE CASCADE
    ) ENGINE=InnoDB;
]]

ok, err = db:multi_query(ddl_schema)
if not ok then
    error("[ERROR] Falló la creación de las tablas: " .. tostring(err))
end
print("[INFO] Tablas creadas/aseguradas exitosamente.")

-- 4. Generación Dinámica de Datos Aleatorios usando SqlValue de cmariadb
math.randomseed(os.time())
local SqlValue = cmariadb.SqlValue

local function rand_elt(arr) return arr[math.random(#arr)] end
local function rand_num(min, max) return math.random(min, max) end

local nombres_personas = {"Carlos", "Ana", "Luis", "María", "Jorge", "Sofía", "Diego", "Valentina", "Mateo", "Camila"}
local apellidos = {"Mendoza", "Gómez", "Torres", "Ortiz", "Silva", "Hernández", "Ramírez", "Juárez", "Ortega", "Ríos"}
local calles = {"Av. Universidad", "Calle San Jerónimo", "Insurgentes Sur", "Reforma", "División del Norte", "Av. Juárez"}
local marcas_cat = {"Burger Empire", "Taco Express", "Wok & Roll", "Pizza Lab", "Green Bowl Salads"}
local categorias_cat = {"Hamburguesas", "Tacos y Antojitos", "Comida Asiática", "Pizzas Artesanales", "Ensaladas y Bebidas"}
local plataformas = {"UberEats", "Rappi", "DidiFood", "WebPropia"}
local estados = {"Pendiente", "En Preparacion", "En Camino", "Entregado", "Cancelado"}
local vehiculos = {"Motocicleta", "Bicicleta Electrica", "Automóvil"}

local marcas_vals, categorias_vals, productos_vals = {}, {}, {}
local ingredientes_vals, recetas_vals, clientes_vals = {}, {}, {}
local repartidores_vals, pedidos_vals, detalle_vals = {}, {}, {}

-- A. Marcas y Categorías
-- A. Marcas y Categorías
for i = 1, #marcas_cat, 1 do
    local v_marca = SqlValue.string(marcas_cat[i])
    local v_cat   = SqlValue.string(categorias_cat[i])
    -- Agregamos comillas simples '%s' alrededor del tostring para que sea un string SQL válido
    table.insert(marcas_vals, string.format("('%s', TRUE)", tostring(v_marca)))
    table.insert(categorias_vals, string.format("('%s')", tostring(v_cat)))
end

-- B. Productos
local nombres_prod = {"Doble", "Especial", "Crujiente", "Suprema", "Mix", "Tradi", "Mega", "Deluxe"}
for i = 1, 10, 1 do
    local c_id = SqlValue.integer(rand_num(1, #categorias_cat))
    local m_id = SqlValue.integer(rand_num(1, #marcas_cat))
    local precio_val = 80 + math.random() * (250 - 80)
    local v_precio = SqlValue.decimal(precio_val)
    
    -- Uso del nuevo método :tonumber() en lugar de acceder a campos internos
    local cat_index = c_id:tonumber() or 1
    local nom = string.format("%s %s %d", categorias_cat[cat_index], rand_elt(nombres_prod), i)
    local v_nom = SqlValue.string(nom)
    
    table.insert(productos_vals, string.format("(%s, %s, '%s', %s)", tostring(m_id), tostring(c_id), tostring(v_nom), tostring(v_precio)))
end

-- C. Ingredientes
local base_ing = {"Carne", "Queso", "Tortilla", "Pollo", "Masa", "Salsa", "Verdura", "Arroz"}
for i = 1, 8, 1 do
    local nom = string.format("%s %s", rand_elt(base_ing), rand_elt(apellidos))
    local um = (i == 3 or i == 5) and "Pieza" or "Kg"
    local stock_val = 20 + math.random() * (100 - 20)
    
    table.insert(ingredientes_vals, string.format("('%s', '%s', %s, 5.000)", 
        tostring(SqlValue.string(nom)), 
        tostring(SqlValue.string(um)), 
        tostring(SqlValue.decimal(stock_val))
    ))
end

-- D. Recetas
for i = 1, 10, 1 do
    local prod_id = SqlValue.integer(i)
    local ing_id  = SqlValue.integer(rand_num(1, 8))
    local cant_req = SqlValue.decimal(0.05 + math.random() * (0.5 - 0.05))
    table.insert(recetas_vals, string.format("(%s, %s, %s)", tostring(prod_id), tostring(ing_id), tostring(cant_req)))
end

-- E. Clientes y Repartidores
for i = 1, 8, 1 do
    local nom_cli = string.format("%s %s", rand_elt(nombres_personas), rand_elt(apellidos))
    local dir = string.format("%s %d", rand_elt(calles), rand_num(10, 999))
    
    -- Añadidas comillas simples para nombre, teléfono y dirección
    table.insert(clientes_vals, string.format("('%s', '%s', '%s')", 
        tostring(SqlValue.string(nom_cli)), 
        tostring(SqlValue.string(string.format("555%07d", math.random(0, 9999999)))), 
        tostring(SqlValue.string(dir))
    ))

    local nom_rep = string.format("%s %s", rand_elt(nombres_personas), rand_elt(apellidos))
    
    -- Añadidas comillas simples para nombre, teléfono y vehículo (quitando las comillas del arreglo original de vehiculos si las tenía)
    table.insert(repartidores_vals, string.format("('%s', '%s', '%s')", 
        tostring(SqlValue.string(nom_rep)), 
        tostring(SqlValue.string(string.format("555%07d", math.random(0, 9999999)))), 
        tostring(SqlValue.string(rand_elt(vehiculos)))
    ))
end

-- F. Pedidos y Detalle
for i = 1, 15, 1 do
    local cli_id = SqlValue.integer(rand_num(1, 8))
    local rep_id = SqlValue.integer(rand_num(1, 8))
    local cant   = SqlValue.integer(rand_num(1, 3))
    local prod_id= SqlValue.integer(rand_num(1, 10))
    
    local p_unit_val = 90 + math.random() * (200 - 90)
    local v_p_unit   = SqlValue.decimal(p_unit_val)
    
    local total_val  = p_unit_val * cant:tonumber() 
    local v_total    = SqlValue.decimal(total_val)
    
    local f_hora_str = string.format("2026-08-%02d %02d:%02d:00", rand_num(1, 24), rand_num(10, 22), rand_num(0, 59))
    local v_datetime = SqlValue.datetime(f_hora_str)
    
    -- Nota: aseguramos comillas para plataforma y estado (si en tu arreglo ya tenían comillas, puedes ajustar o dejarlas limpias aquí)
    table.insert(
        pedidos_vals,
        string.format("(%s, %s, '%s', '%s', '%s', %s)",
            tostring(cli_id), tostring(rep_id), rand_elt(plataformas), tostring(v_datetime), rand_elt(estados), tostring(v_total)
        )
    )
        
    table.insert(detalle_vals, string.format("(%s, %s, %s, %s)", 
        tostring(SqlValue.integer(i)), tostring(prod_id), tostring(cant), tostring(v_p_unit)
    ))
end

-- Ensamblar script DML final con control de FKs
local dml_inserts = string.format([[
    SET FOREIGN_KEY_CHECKS = 0;

    INSERT INTO marcas (nombre, activa) VALUES %s
    ON DUPLICATE KEY UPDATE nombre=VALUES(nombre);

    INSERT INTO categorias (nombre) VALUES %s
    ON DUPLICATE KEY UPDATE nombre=VALUES(nombre);

    INSERT INTO productos (id_marca, id_categoria, nombre, precio_venta) VALUES %s
    ON DUPLICATE KEY UPDATE precio_venta=VALUES(precio_venta);

    INSERT INTO ingredientes (nombre, unidad_medida, stock_actual, stock_minimo) VALUES %s
    ON DUPLICATE KEY UPDATE stock_actual=VALUES(stock_actual);

    INSERT INTO recetas (id_producto, id_ingrediente, cantidad_requerida) VALUES %s
    ON DUPLICATE KEY UPDATE cantidad_requerida=VALUES(cantidad_requerida);

    INSERT INTO clientes (nombre, telefono, direccion) VALUES %s
    ON DUPLICATE KEY UPDATE telefono=VALUES(telefono);

    INSERT INTO repartidores (nombre, telefono, vehiculo) VALUES %s
    ON DUPLICATE KEY UPDATE vehiculo=VALUES(vehiculo);

    INSERT INTO pedidos (id_cliente, id_repartidor, plataforma_origen, fecha_hora, estado, total) VALUES %s
    ON DUPLICATE KEY UPDATE estado=VALUES(estado);

    INSERT INTO detalle_pedidos (id_pedido, id_producto, cantidad, precio_unitario) VALUES %s
    ON DUPLICATE KEY UPDATE cantidad=VALUES(cantidad);

    SET FOREIGN_KEY_CHECKS = 1;
]], 
    table.concat(marcas_vals, ","),
    table.concat(categorias_vals, ","),
    table.concat(productos_vals, ","),
    table.concat(ingredientes_vals, ","),
    table.concat(recetas_vals, ","),
    table.concat(clientes_vals, ","),
    table.concat(repartidores_vals, ","),
    table.concat(pedidos_vals, ","),
    table.concat(detalle_vals, ",")
)

local ok, err = db:multi_query(dml_inserts)
if not ok then
    error("[ERROR] Al insertar datos aleatorios: " .. tostring(err))
end
print("[INFO] Datos semilla aleatorios insertados correctamente usando SqlValue.")

-- 5. Consulta de verificación
print("\n--- RESUMEN DE PRODUCTOS POR MARCA ---")
local res = db:query("SELECT * FROM productos;")--[=[db:multi_query([[
    SELECT p.id_producto, m.nombre AS marca, p.nombre AS producto, p.precio_venta 
    FROM productos p 
    JOIN marcas m ON p.id_marca = m.id_marca;
]])]=]
system.print(res)
db:close()

--[[
ALTER USER 'lua_client'@'localhost' IDENTIFIED VIA mysql_native_password USING PASSWORD('12345');
CREATE USER IF NOT EXISTS 'lua_client'@'127.0.0.1' IDENTIFIED VIA mysql_native_password USING PASSWORD('12345');
GRANT ALL PRIVILEGES ON *.* TO 'lua_client'@'127.0.0.1' WITH GRANT OPTION;
FLUSH PRIVILEGES;
EXIT;
]]