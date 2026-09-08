import("cmariadb", "system")

local db = assert(cmariadb.connect({
    host = "127.0.0.1",
    user = "lua_client",
    password = "12345",
    client_mode = cmariadb.CLIENT_MODE.MULTIPLE_STATEMENTS
}))

assert(db:query("DROP DATABASE IF EXISTS coffees;"))
local resultado = assert(db:multi_query([[
    CREATE DATABASE IF NOT EXISTS coffees CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
    USE coffees;
    CREATE TABLE IF NOT EXISTS productos (
        id_producto INT PRIMARY KEY AUTO_INCREMENT,
        nombre VARCHAR(150) UNIQUE,
        precio DECIMAL(5,2) NOT NULL,
        existencia INT NOT NULL,
        codigo VARCHAR(8) UNIQUE,
        categoria ENUM('Bebidas', 'Alimentos'),
        activo BOOLEAN DEFAULT TRUE,
        marca VARCHAR(50) UNIQUE
    );

    CREATE TABLE IF NOT EXISTS precios (
        id_precio INT PRIMARY KEY AUTO_INCREMENT,
        id_producto INT NOT NULL,
        precio_actual DECIMAL(5,2) NOT NULL,
        fecha_actualizacion TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        CONSTRAINT fk_precio_producto FOREIGN KEY (id_producto) 
            REFERENCES productos(id_producto) 
            ON DELETE CASCADE 
            ON UPDATE CASCADE
    );

    CREATE TABLE IF NOT EXISTS ventas (
        id_venta INT PRIMARY KEY AUTO_INCREMENT,
        id_producto INT NOT NULL,
        cantidad INT NOT NULL,
        total DECIMAL(8,2) NOT NULL,
        fecha_venta DATETIME DEFAULT CURRENT_TIMESTAMP,
        CONSTRAINT fk_venta_producto FOREIGN KEY (id_producto) 
            REFERENCES productos(id_producto) 
            ON DELETE CASCADE 
            ON UPDATE CASCADE
    );
]]))

local productos = {}
productos.cafes = {
    sufijo = "Café",
    variantes = {"Expreso", "Americano", "Cappuchino", "Moka", "Late"},
    categoria = "Bebidas"
}
productos.galletas = {
    sufijo = "Galleta",
    variantes = {"de Nuez", "de Chocolate", "Rellana de Chocolate", "Maíz"},
    categoria = "Alimentos"
}
productos.tes = {
    sufijo = "Té",
    variantes = {"Blanco", "Negro", "Manzanilla", "Verde", "de Menta"},
    categoria = "Bebidas"
}
productos.panes = {
    sufijo = "Pan",
    variantes = {"de Nata", "de Nuez", "de Queso"},
    categoria = "Alimentos"
}
productos.bagels = {
    sufijo = "Bagel",
    variantes = {"de Salmón y Aguacate", "de Pavo y Aguacate", "de Tocino y Huevo"},
    categoria = "Alimentos"
}
productos.sandwiches = {
    sufijo = "Sandwich",
    variantes = {"Cubano", "Mixto", "de Atún", "de Queso fundido"},
    categoria = "Alimentos"
}

math.randomseed(os.time())
local marcas_disponibles = {"StarCoffee", "Nescafe", "Bimbo", "Marías", "Lipton", "Bienestar", "La Especial"}
local total_productos = 0
local codigos_generados = {}
local function crearCodigo() return string.format("C%06d%d", math.random(1, 999999), math.random(0, 9)) end

for _, grupo in pairs(productos) do
    for _, variante in ipairs(grupo.variantes) do
        local nombre = grupo.sufijo .. " " .. variante
        local precio = math.random(2000, 15000) / 100 -- Precio entre 20.00 y 150.00
        local existencia = math.random(10, 100)
        local codigo = crearCodigo()
        while codigos_generados[codigo] do codigo = crearCodigo() end
        codigos_generados[codigo] = codigo

        local marca = marcas_disponibles[math.random(#marcas_disponibles)] .. " " .. math.random(1, 500)
        local query_prod = string.format(
            "INSERT INTO productos (nombre, precio, existencia, codigo, categoria, marca) VALUES ('%s', %.2f, %d, '%s', '%s', '%s');",
            nombre, precio, existencia, codigo, grupo.categoria, marca
        )
        assert(db:query(query_prod))
        
        local res_id = assert(db:query("SELECT LAST_INSERT_ID() as id;"))
        local id_producto = res_id[1].id
        local query_precio = string.format(
            "INSERT INTO precios (id_producto, precio_actual) VALUES (%d, %.2f);",
            id_producto:tostring(), precio
        )
        assert(db:query(query_precio))
        total_productos += 1
    end
end

-- Función auxiliar para generar un DATETIME aleatorio en los últimos 30 días
local function generar_datetime_aleatorio()
    local tiempo_actual = os.time()
    local dias_atras_segundos = math.random(0, 30 * 24 * 3600)
    local timestamp_rand = tiempo_actual - dias_atras_segundos
    return os.date("%Y-%m-%d %H:%M:%S", timestamp_rand)
end

-- Obtener todos los IDs de productos generados reales
local res_prods = assert(db:query("SELECT id_producto FROM productos;"))

for i = 1, total_productos, 1 do
    local prod_random = res_prods[math.random(1, #res_prods)]
    local id_prod = prod_random.id_producto:value()
    local cantidad = math.random(20)
    
    local res_p = assert(db:query(string.format("SELECT precio FROM productos WHERE id_producto = %d;", id_prod)))
    if #res_p > 0 then
        local precio_unitario = res_p[1].precio:value()
        local total = precio_unitario * cantidad
        local fecha_rand = generar_datetime_aleatorio()
        
        local query_venta = string.format(
            "INSERT INTO ventas (id_producto, cantidad, total, fecha_venta) VALUES (%d, %d, %.2f, '%s');",
            id_prod, cantidad, total, fecha_rand
        )
        assert(db:query(query_venta))
    end
end

print("Registros en la tabla ventas con DATETIME aleatorio:")
local resultado_filtro = assert(db:query("SELECT * FROM ventas ORDER BY fecha_venta DESC;"))
system.print(resultado_filtro)

db:close()