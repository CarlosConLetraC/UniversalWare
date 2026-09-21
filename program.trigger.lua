import("cmariadb", "system")

local db = assert(cmariadb.connect({
    host = "127.0.0.1",
    user = "lua_client",
    password = "12345",
    db = "coffees"
}))

assert(db:execute([[
    DROP TRIGGER IF EXISTS trg_validar_precio_producto;

    DELIMITER //

    CREATE TRIGGER trg_validar_precio_producto
    BEFORE INSERT ON productos
    FOR EACH ROW
    BEGIN
        IF NEW.precio <= 0 THEN
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'Error: El precio del producto debe ser mayor a 0.';
        END IF;
    END //

    DELIMITER ;
]]))

print("[OK] Trigger 'trg_validar_precio_producto' creado exitosamente para la base de datos 'coffees'.")

db:close()