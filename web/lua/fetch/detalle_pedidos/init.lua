local M = {}

function M.get(db, cjson, sanitizar, peticion, escape_sql)
    local res = assert(db:query("SELECT id_pedido, id_producto, cantidad, precio_unitario FROM detalle_pedidos ORDER BY id_pedido ASC, id_producto ASC;"))
    peticion:respond(200, cjson.encode(sanitizar(res)))
end

function M.post(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id_pedido = tonumber(body.id_pedido)
    local id_producto = tonumber(body.id_producto)
    local cantidad = tonumber(body.cantidad)
    local precio_unitario = tonumber(body.precio_unitario)
    if not id_pedido or not id_producto or not cantidad or not precio_unitario then error("Parámetros numéricos inválidos") end

    local sql = string.format("INSERT INTO detalle_pedidos (id_pedido, id_producto, cantidad, precio_unitario) VALUES (%d, %d, %d, %.2f);",
        id_pedido, id_producto, cantidad, precio_unitario)
    assert(db:query(sql))
    peticion:respond(201, cjson.encode({ status = "ok", message = "Detalle creado" }))
end

function M.put(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id_pedido = tonumber(body.id_pedido)
    local id_producto = tonumber(body.id_producto)
    local cantidad = tonumber(body.cantidad)
    local precio_unitario = tonumber(body.precio_unitario)
    if not id_pedido or not id_producto or not cantidad or not precio_unitario then error("Parámetros numéricos inválidos") end

    local sql = string.format("UPDATE detalle_pedidos SET cantidad = %d, precio_unitario = %.2f WHERE id_pedido = %d AND id_producto = %d;",
        cantidad, precio_unitario, id_pedido, id_producto)
    assert(db:query(sql))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Detalle actualizado" }))
end

function M.delete(db, cjson, sanitizar, peticion, escape_sql)
    local id_ped = tonumber(peticion.path:match("id_pedido=(%d+)"))
    local id_prod = tonumber(peticion.path:match("id_producto=(%d+)"))
    if not id_ped or not id_prod then error("Faltan parámetros de la clave compuesta") end

    assert(db:query(string.format("DELETE FROM detalle_pedidos WHERE id_pedido = %d AND id_producto = %d;", id_ped, id_prod)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Detalle eliminado" }))
end

return M