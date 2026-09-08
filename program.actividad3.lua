import("cmariadb", "system")

-- 1. Conexión a la base de datos
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

-- Creación y selección de la base de datos
assert(db:query("DROP DATABASE IF EXISTS autos_concesionario_db;"))
assert(db:query("CREATE DATABASE IF NOT EXISTS autos_concesionario_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"))
assert(db:query("USE autos_concesionario_db;"))
print("[INFO] Base de datos 'autos_concesionario_db' creada y seleccionada.")

-- 2. Definición del Esquema DDL (Relaciones, Llaves Primarias y Foráneas)
local ddl_schema = [[
    CREATE TABLE IF NOT EXISTS concesionarios (
        id_concesionario INT AUTO_INCREMENT PRIMARY KEY,
        nombre VARCHAR(100) NOT NULL,
        domicilio VARCHAR(200) NOT NULL,
        telefono VARCHAR(20) NOT NULL
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS vendedores (
        id_vendedor INT AUTO_INCREMENT PRIMARY KEY,
        id_concesionario INT NOT NULL,
        nombre VARCHAR(100) NOT NULL,
        domicilio VARCHAR(200) NOT NULL,
        telefono VARCHAR(20) NOT NULL,
        whatsapp VARCHAR(20) NOT NULL,
        FOREIGN KEY (id_concesionario) REFERENCES concesionarios(id_concesionario) ON DELETE CASCADE ON UPDATE CASCADE
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS modelos_autos (
        id_modelo INT AUTO_INCREMENT PRIMARY KEY,
        marca VARCHAR(50) NOT NULL,
        modelo VARCHAR(50) NOT NULL,
        anho INT NOT NULL,
        precio_base DECIMAL(12,2) NOT NULL,
        descuento DECIMAL(5,2) DEFAULT 0.00,
        tipo_motor VARCHAR(50) NOT NULL,
        potencia INT NOT NULL,
        cilindros INT NOT NULL
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS automoviles_stock (
        numero_serie VARCHAR(50) PRIMARY KEY,
        id_modelo INT NOT NULL,
        id_concesionario INT NOT NULL,
        ubicacion VARCHAR(50) NOT NULL, -- Local, Sucursal, Almacen, Servicio Oficial
        FOREIGN KEY (id_modelo) REFERENCES modelos_autos(id_modelo) ON DELETE CASCADE ON UPDATE CASCADE,
        FOREIGN KEY (id_concesionario) REFERENCES concesionarios(id_concesionario) ON DELETE CASCADE ON UPDATE CASCADE
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS ventas (
        id_venta INT AUTO_INCREMENT PRIMARY KEY,
        numero_serie VARCHAR(50) NOT NULL,
        id_vendedor INT NOT NULL,
        precio_venta DECIMAL(12,2) NOT NULL,
        modo_pago ENUM('Contado', 'Credito') NOT NULL,
        fecha_entrega DATE NOT NULL,
        matricula VARCHAR(20) NOT NULL,
        origen_adquisicion ENUM('Stock', 'Fabrica') NOT NULL,
        FOREIGN KEY (numero_serie) REFERENCES automoviles_stock(numero_serie) ON DELETE RESTRICT ON UPDATE CASCADE,
        FOREIGN KEY (id_vendedor) REFERENCES vendedores(id_vendedor) ON DELETE RESTRICT ON UPDATE CASCADE
    ) ENGINE=InnoDB;

    CREATE TABLE IF NOT EXISTS ventas_acumuladas (
        id_simulacion INT AUTO_INCREMENT PRIMARY KEY,
        fecha DATE NOT NULL,
        modelo VARCHAR(100) NOT NULL,
        vendedor VARCHAR(100) NOT NULL,
        total DECIMAL(12,2) NOT NULL
    ) ENGINE=InnoDB;
]]

assert(db:multi_query(ddl_schema))
print("[INFO] Estructura relacional creada exitosamente.")

-- 3. Inserción automatizada de 20 registros NOT NULL por tabla
math.randomseed(os.time())

local marcas_arr = {"Audi", "Suzuki", "Honda", "Volkswagen", "BMW", "Toyota", "Nissan", "Mazda"}
local nombres_arr = {"Carlos", "Ana", "Luis", "Maria", "Jorge", "Sofia", "Diego", "Valeria"}
local apellidos_arr = {"Gomez", "Lopez", "Perez", "Martines", "Sanchez", "Ramirez"}

local function rand_elt(t) return t[math.random(#t)] end

-- Generar datos en bloques
local con_vals, vend_vals, mod_vals, auto_vals, vent_vals, sim_vals = {}, {}, {}, {}, {}, {}

for i = 1, 20, 1 do
    -- Concesionarios
    table.insert(con_vals, string.format("('Concesionario Central %d', 'Av. Reforma %d', '555000%02d')", i, i*10, i))
    
    -- Vendedores (asociados a concesionarios 1 a 20)
    table.insert(vend_vals, string.format("(%d, '%s %s', 'Calle Sur %d', '555111%02d', '555111%02d')", 
        ((i-1)%5)+1, rand_elt(nombres_arr), rand_elt(apellidos_arr), i, i, i))
    
    -- Modelos de Autos (mezclando años antes y después de 2020 para la prueba de DELETE)
    local anho_val = (i <= 5) and (2017 + (i % 3)) or 2021 + (i % 5)
    table.insert(mod_vals, string.format("('%s', 'Modelo-%d', %d, %d.00, %d.00, 'Turbo', %d, %d)", 
        rand_elt(marcas_arr), i, anho_val, 140000 + (i * 12000), (i%2==0) and 5.00 or 0.00, 150 + i*10, (i%2==0)?4:6))
    
    -- Automóviles en Stock
    table.insert(auto_vals, string.format("('SN-VIN-%04d', %d, %d, '%s')", 
        i, i, ((i-1)%5)+1, (i%2==0) and "Local Ventas" or "Servicio Oficial"))
        
    -- Ventas
    table.insert(vent_vals, string.format("('SN-VIN-%04d', %d, %d.00, '%s', '2026-08-%02d', 'ABC-%03d', '%s')", 
        i, ((i-1)%5)+1, 150000.00 + (i * 5000), (i%2==0) and "Contado" or "Credito", (i%28)+1, i, (i%3==0) and "Fabrica" or "Stock"))
end

-- 25 Simulaciones para ventas_acumuladas
for i = 1, 25, 1 do
    table.insert(sim_vals, string.format("('2026-08-%02d', 'Modelo-%d', 'Vendedor %d', %d.00)", 
        (i%28)+1, (i%20)+1, (i%5)+1, 140000 + (i * 8500)))
end

local dml_inserts = string.format([[
    SET FOREIGN_KEY_CHECKS = 0;
    INSERT INTO concesionarios (nombre, domicilio, telefono) VALUES %s;
    INSERT INTO vendedores (id_concesionario, nombre, domicilio, telefono, whatsapp) VALUES %s;
    INSERT INTO modelos_autos (marca, modelo, anho, precio_base, descuento, tipo_motor, potencia, cilindros) VALUES %s;
    INSERT INTO automoviles_stock (numero_serie, id_modelo, id_concesionario, ubicacion) VALUES %s;
    INSERT INTO ventas (numero_serie, id_vendedor, precio_venta, modo_pago, fecha_entrega, matricula, origen_adquisicion) VALUES %s;
    INSERT INTO ventas_acumuladas (fecha, modelo, vendedor, total) VALUES %s;
    SET FOREIGN_KEY_CHECKS = 1;
]], table.concat(con_vals, ","), table.concat(vend_vals, ","), table.concat(mod_vals, ","), 
    table.concat(auto_vals, ","), table.concat(vent_vals, ","), table.concat(sim_vals, ","))

assert(db:multi_query(dml_inserts))
print("[INFO] Registros semilla insertados correctamente en todas las tablas.")

-- ==========================================
-- 4. EJECUCIÓN DE CONSULTAS Y OPERACIONES SOLICITADAS
-- ==========================================

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

print("\n--- A. VENTAS DE UNA SOLA CONCESIONARIA (LIMIT 5) ---")
local q_limit = sanitizar_tabla(db:query([[
    SELECT v.id_venta, c.nombre AS concesionario, v.precio_venta, v.modo_pago, v.matricula 
    FROM ventas v
    JOIN automoviles_stock a ON v.numero_serie = a.numero_serie
    JOIN concesionarios c ON a.id_concesionario = c.id_concesionario
    WHERE c.id_concesionario = 1
    LIMIT 5;
]]))
system.print(q_limit)

print("\n--- B. VENTAS ORDENADAS MAYORES A $150,000.00 (ORDER BY) ---")
local q_order = sanitizar_tabla(db:query([[
    SELECT id_venta, numero_serie, precio_venta, modo_pago, fecha_entrega 
    FROM ventas 
    WHERE precio_venta > 150000.00 
    ORDER BY precio_venta DESC;
]]))
system.print(q_order)

print("\n--- C. MODIFICAR NOMBRE DE CONCESIONARIOS CON REPLACE ---")
assert(db:query([[
    UPDATE concesionarios 
    SET nombre = CONCAT(nombre, '-nacional');
]]))
local q_replace = sanitizar_tabla(db:query("SELECT id_concesionario, nombre FROM concesionarios LIMIT 5;"))
system.print(q_replace)
print("> Interpretación: Se actualizó el nombre de cada concesionaria concatenando el sufijo nacional.")

print("\n--- D. ELIMINAR MODELOS ANTERIORES AL AÑO 2020 (DELETE) ---")
assert(db:multi_query([[
    SET FOREIGN_KEY_CHECKS = 0;
    DELETE FROM ventas WHERE numero_serie IN (
        SELECT s.numero_serie FROM automoviles_stock s 
        JOIN modelos_autos m ON s.id_modelo = m.id_modelo 
        WHERE m.anho < 2020
    );
    DELETE FROM automoviles_stock WHERE id_modelo IN (
        SELECT id_modelo FROM modelos_autos WHERE anho < 2020
    );
    DELETE FROM modelos_autos WHERE anho < 2020;
    SET FOREIGN_KEY_CHECKS = 1;
]]))
assert(db:query("DELETE FROM modelos_autos WHERE anho < 2020;"))
local q_delete = db:query("SELECT id_modelo, marca, modelo, anho FROM modelos_autos LIMIT 5;")
system.print(q_delete)
print("> Interpretación: Se removieron del catálogo todos los vehículos con año de fabricación menor a 2020.")

db:close()