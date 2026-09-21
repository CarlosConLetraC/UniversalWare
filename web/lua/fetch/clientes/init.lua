local M = {}

function M.get(db, cjson, sanitizar, peticion, escape_sql)
    local res = assert(db:query("SELECT id_cliente, nombre, telefono, direccion FROM clientes ORDER BY id_cliente ASC;"))
    peticion:respond(200, cjson.encode(sanitizar(res)))
end

function M.post(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local nombre = escape_sql(body.nombre)
    local telefono = escape_sql(body.telefono)
    local direccion = escape_sql(body.direccion)

    assert(db:query(string.format("INSERT INTO clientes (nombre, telefono, direccion) VALUES ('%s', '%s', '%s');",
        nombre, telefono, direccion)))
    peticion:respond(201, cjson.encode({ status = "ok", message = "Cliente creado" }))
end

function M.put(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id = tonumber(body.id_cliente)
    if not id then error("ID inválido o faltante") end

    local nombre = escape_sql(body.nombre)
    local telefono = escape_sql(body.telefono)
    local direccion = escape_sql(body.direccion)

    assert(db:query(string.format("UPDATE clientes SET nombre = '%s', telefono = '%s', direccion = '%s' WHERE id_cliente = %d;",
        nombre, telefono, direccion, id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Cliente actualizado" }))
end

function M.delete(db, cjson, sanitizar, peticion, escape_sql)
    local raw_id = peticion.path:match("id_cliente=(%d+)")
    local id = tonumber(raw_id)
    if not id then error("Parámetro id_cliente faltante o inválido") end

    assert(db:query(string.format("DELETE FROM clientes WHERE id_cliente = %d;", id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Cliente eliminado" }))
end

return M