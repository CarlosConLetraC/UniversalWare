local M = {}

function M.get(db, cjson, sanitizar, peticion, escape_sql)
    local res = assert(db:query("SELECT id_repartidor, nombre, telefono, vehiculo FROM repartidores ORDER BY id_repartidor ASC;"))
    peticion:respond(200, cjson.encode(sanitizar(res)))
end

function M.post(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local nombre = escape_sql(body.nombre)
    local telefono = escape_sql(body.telefono)
    local vehiculo = escape_sql(body.vehiculo)

    assert(db:query(string.format("INSERT INTO repartidores (nombre, telefono, vehiculo) VALUES ('%s', '%s', '%s');",
        nombre, telefono, vehiculo)))
    peticion:respond(201, cjson.encode({ status = "ok", message = "Repartidor creado" }))
end

function M.put(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id = tonumber(body.id_repartidor)
    if not id then error("ID inválido o faltante") end

    local nombre = escape_sql(body.nombre)
    local telefono = escape_sql(body.telefono)
    local vehiculo = escape_sql(body.vehiculo)

    assert(db:query(string.format("UPDATE repartidores SET nombre = '%s', telefono = '%s', vehiculo = '%s' WHERE id_repartidor = %d;",
        nombre, telefono, vehiculo, id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Repartidor actualizado" }))
end

function M.delete(db, cjson, sanitizar, peticion, escape_sql)
    local raw_id = peticion.path:match("id_repartidor=(%d+)")
    local id = tonumber(raw_id)
    if not id then error("Parámetro id_repartidor faltante o inválido") end

    assert(db:query(string.format("DELETE FROM repartidores WHERE id_repartidor = %d;", id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Repartidor eliminado" }))
end

return M