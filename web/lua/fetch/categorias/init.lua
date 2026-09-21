local M = {}

function M.get(db, cjson, sanitizar, peticion, escape_sql)
    local res = assert(db:query("SELECT id_categoria, nombre FROM categorias ORDER BY id_categoria ASC;"))
    peticion:respond(200, cjson.encode(sanitizar(res)))
end

function M.post(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local nombre = escape_sql(body.nombre)

    assert(db:query(string.format("INSERT INTO categorias (nombre) VALUES ('%s');", nombre)))
    peticion:respond(201, cjson.encode({ status = "ok", message = "Categoría creada" }))
end

function M.put(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id = tonumber(body.id_categoria)
    if not id then error("ID inválido o faltante") end

    local nombre = escape_sql(body.nombre)

    assert(db:query(string.format("UPDATE categorias SET nombre = '%s' WHERE id_categoria = %d;", nombre, id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Categoría actualizada" }))
end

function M.delete(db, cjson, sanitizar, peticion, escape_sql)
    local raw_id = peticion.path:match("id_categoria=(%d+)")
    local id = tonumber(raw_id)
    if not id then error("Parámetro id_categoria faltante o inválido") end

    assert(db:query(string.format("DELETE FROM categorias WHERE id_categoria = %d;", id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Categoría eliminada" }))
end

return M