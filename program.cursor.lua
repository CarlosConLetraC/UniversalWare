import("cmariadb", "system")

local db = assert(cmariadb.connect({
    host = "127.0.0.1",
    user = "lua_client",
    password = "12345",
    db = "coffees"
}))

assert(db:execute([[
    DROP PROCEDURE IF EXISTS sp_cursor_productos_por_categoria;

    DELIMITER //

    CREATE PROCEDURE sp_cursor_productos_por_categoria(
        IN p_categoria VARCHAR(50),
        IN p_limite INT
    )
    BEGIN
        -- 1. Declaración de variables locales
        DECLARE v_codigo VARCHAR(8);
        DECLARE v_nombre VARCHAR(150);
        DECLARE v_existencia INT;
        DECLARE done INT DEFAULT 0;

        -- 2. Declaración del cursor con LIMIT dinámico
        DECLARE c_productos CURSOR FOR 
            SELECT codigo, nombre, existencia 
            FROM productos 
            WHERE categoria = p_categoria AND activo = 1 
            ORDER BY nombre 
            LIMIT p_limite;

        -- 3. Declaración de manejadores (HANDLER)
        DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = 1;

        -- 4. Sentencias ejecutables (Tablas temporales, DELETE, etc.)
        CREATE TEMPORARY TABLE IF NOT EXISTS tmp_cursor_resultado (
            codigo VARCHAR(8),
            nombre VARCHAR(150),
            existencia INT
        );
        DELETE FROM tmp_cursor_resultado;

        -- 5. Abrir e iterar el cursor
        OPEN c_productos;

        read_loop: LOOP
            FETCH c_productos INTO v_codigo, v_nombre, v_existencia;
            IF done = 1 THEN
                LEAVE read_loop;
            END IF;

            INSERT INTO tmp_cursor_resultado (codigo, nombre, existencia) 
            VALUES (v_codigo, v_nombre, v_existencia);
        END LOOP;

        CLOSE c_productos;
    END //

    DELIMITER ;
]]))

assert(db:query("CALL sp_cursor_productos_por_categoria('Bebidas', 5);"))

local resultados = assert(db:query("SELECT * FROM tmp_cursor_resultado;"))
print("--- PRODUCTOS PROCESADOS MEDIANTE CURSOR (Bebidas, Límite: 5) ---")
system.print(resultados)

db:close()