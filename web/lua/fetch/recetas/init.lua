local M = {}

function M.get(db, cjson, sanitizar, peticion, escape_sql)
    local res = assert(db:query("SELECT id_producto, id_ingrediente, cantidad_requerida FROM recetas ORDER BY id_producto ASC, id_ingrediente ASC;"))
    peticion:respond(200, cjson.encode(sanitizar(res)))
end

function M.post(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id_producto = tonumber(body.id_producto)
    local id_ingrediente = tonumber(body.id_ingrediente)
    local cantidad_requerida = tonumber(body.cantidad_requerida)
    if not id_producto or not id_ingrediente or not cantidad_requerida then error("Parámetros numéricos inválidos") end

    local sql = string.format("INSERT INTO recetas (id_producto, id_ingrediente, cantidad_requerida) VALUES (%d, %d, %.3f);",
        id_producto, id_ingrediente, cantidad_requerida)
    assert(db:query(sql))
    peticion:respond(201, cjson.encode({ status = "ok", message = "Receta creada" }))
end

function M.put(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id_producto = tonumber(body.id_producto)
    local id_ingrediente = tonumber(body.id_ingrediente)
    local cantidad_requerida = tonumber(body.cantidad_requerida)
    if not id_producto or not id_ingrediente or not cantidad_requerida then error("Parámetros numéricos inválidos") end

    local sql = string.format("UPDATE recetas SET cantidad_requerida = %.3f WHERE id_producto = %d AND id_ingrediente = %d;",
        cantidad_requerida, id_producto, id_ingrediente)
    assert(db:query(sql))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Receta actualizada" }))
end

function M.delete(db, cjson, sanitizar, peticion, escape_sql)
    local id_prod = tonumber(peticion.path:match("id_producto=(%d+)"))
    local id_ing = tonumber(peticion.path:match("id_ingrediente=(%d+)"))
    if not id_prod or not id_ing then error("Faltan parámetros de la clave compuesta") end

    assert(db:query(string.format("DELETE FROM recetas WHERE id_producto = %d AND id_ingrediente = %d;", id_prod, id_ing)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Receta eliminada" }))
end

return M