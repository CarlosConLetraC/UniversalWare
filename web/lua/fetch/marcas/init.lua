local M = {}

function M.get(db, cjson, sanitizar, peticion, escape_sql)
    local res = assert(db:query("SELECT id_marca, nombre, activa FROM marcas ORDER BY id_marca ASC;"))
    peticion:respond(200, cjson.encode(sanitizar(res)))
end

function M.post(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local nombre = escape_sql(body.nombre)
    local activa = tonumber(body.activa) or 1

    local sql = string.format("INSERT INTO marcas (nombre, activa) VALUES ('%s', %d);", nombre, activa)
    assert(db:query(sql))
    peticion:respond(201, cjson.encode({ status = "ok", message = "Marca creada" }))
end

function M.put(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id = tonumber(body.id_marca)
    if not id then error("ID inválido o faltante") end

    local nombre = escape_sql(body.nombre)
    local activa = tonumber(body.activa) or 1

    local sql = string.format("UPDATE marcas SET nombre = '%s', activa = %d WHERE id_marca = %d;", nombre, activa, id)
    assert(db:query(sql))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Marca actualizada" }))
end

function M.delete(db, cjson, sanitizar, peticion, escape_sql)
    local raw_id = peticion.path:match("id_marca=(%d+)")
    local id = tonumber(raw_id)
    if not id then error("Parámetro id_marca faltante o inválido") end

    assert(db:query(string.format("DELETE FROM marcas WHERE id_marca = %d;", id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Marca eliminada" }))
end

return M