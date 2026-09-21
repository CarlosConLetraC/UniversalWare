local M = {}

function M.get(db, cjson, sanitizar, peticion, escape_sql)
    local res = assert(db:query("SELECT id_ingrediente, nombre, unidad_medida, stock_actual, stock_minimo FROM ingredientes ORDER BY id_ingrediente ASC;"))
    peticion:respond(200, cjson.encode(sanitizar(res)))
end

function M.post(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local stock_actual = tonumber(body.stock_actual) or 0.0
    local stock_minimo = tonumber(body.stock_minimo) or 1.0

    local nombre = escape_sql(body.nombre)
    local unidad_medida = escape_sql(body.unidad_medida)

    local sql = string.format("INSERT INTO ingredientes (nombre, unidad_medida, stock_actual, stock_minimo) VALUES ('%s', '%s', %.3f, %.3f);",
        nombre, unidad_medida, stock_actual, stock_minimo)
    assert(db:query(sql))
    peticion:respond(201, cjson.encode({ status = "ok", message = "Ingrediente creado" }))
end

function M.put(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id = tonumber(body.id_ingrediente)
    local stock_actual = tonumber(body.stock_actual)
    local stock_minimo = tonumber(body.stock_minimo)
    if not id or not stock_actual or not stock_minimo then error("Parámetros numéricos inválidos") end

    local nombre = escape_sql(body.nombre)
    local unidad_medida = escape_sql(body.unidad_medida)

    local sql = string.format("UPDATE ingredientes SET nombre = '%s', unidad_medida = '%s', stock_actual = %.3f, stock_minimo = %.3f WHERE id_ingrediente = %d;",
        nombre, unidad_medida, stock_actual, stock_minimo, id)
    assert(db:query(sql))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Ingrediente actualizado" }))
end

function M.delete(db, cjson, sanitizar, peticion, escape_sql)
    local raw_id = peticion.path:match("id_ingrediente=(%d+)")
    local id = tonumber(raw_id)
    if not id then error("Parámetro id_ingrediente faltante o inválido") end

    assert(db:query(string.format("DELETE FROM ingredientes WHERE id_ingrediente = %d;", id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Ingrediente eliminado" }))
end

return M