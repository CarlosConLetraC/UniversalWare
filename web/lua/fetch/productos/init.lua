local M = {}

function M.get(db, cjson, sanitizar, peticion, escape_sql)
    local res = assert(db:query("SELECT id_producto, id_marca, id_categoria, nombre, precio_venta FROM productos ORDER BY id_producto ASC;"))
    peticion:respond(200, cjson.encode(sanitizar(res)))
end

function M.post(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id_marca = tonumber(body.id_marca)
    local id_categoria = tonumber(body.id_categoria)
    local precio_venta = tonumber(body.precio_venta)
    if not id_marca or not id_categoria or not precio_venta then error("Parámetros numéricos inválidos") end

    local nombre = escape_sql(body.nombre)

    local sql = string.format("INSERT INTO productos (id_marca, id_categoria, nombre, precio_venta) VALUES (%d, %d, '%s', %.2f);",
        id_marca, id_categoria, nombre, precio_venta)
    assert(db:query(sql))
    peticion:respond(201, cjson.encode({ status = "ok", message = "Producto creado" }))
end

function M.put(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id_producto = tonumber(body.id_producto)
    local id_marca = tonumber(body.id_marca)
    local id_categoria = tonumber(body.id_categoria)
    local precio_venta = tonumber(body.precio_venta)
    if not id_producto or not id_marca or not id_categoria or not precio_venta then error("Parámetros numéricos inválidos") end

    local nombre = escape_sql(body.nombre)

    local sql = string.format("UPDATE productos SET id_marca = %d, id_categoria = %d, nombre = '%s', precio_venta = %.2f WHERE id_producto = %d;",
        id_marca, id_categoria, nombre, precio_venta, id_producto)
    assert(db:query(sql))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Producto actualizado" }))
end

function M.delete(db, cjson, sanitizar, peticion, escape_sql)
    local raw_id = peticion.path:match("id_producto=(%d+)")
    local id = tonumber(raw_id)
    if not id then error("Parámetro id_producto faltante o inválido") end

    assert(db:query(string.format("DELETE FROM productos WHERE id_producto = %d;", id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Producto eliminado" }))
end

return M