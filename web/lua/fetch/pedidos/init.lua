local M = {}

function M.get(db, cjson, sanitizar, peticion, escape_sql)
    local res = assert(db:query("SELECT id_pedido, id_cliente, id_repartidor, plataforma_origen, fecha_hora, estado, total FROM pedidos ORDER BY id_pedido ASC;"))
    peticion:respond(200, cjson.encode(sanitizar(res)))
end

function M.post(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id_cliente = tonumber(body.id_cliente)
    local id_repartidor = tonumber(body.id_repartidor)
    local total = tonumber(body.total)
    if not id_cliente or not id_repartidor or not total then error("Parámetros numéricos inválidos") end

    local plataforma_origen = escape_sql(body.plataforma_origen)
    local fecha = escape_sql(body.fecha_hora or os.date("%Y-%m-%d %H:%M:%S"))
    local estado = escape_sql(body.estado or 'Pendiente')

    local sql = string.format("INSERT INTO pedidos (id_cliente, id_repartidor, plataforma_origen, fecha_hora, estado, total) VALUES (%d, %d, '%s', '%s', '%s', %.2f);",
        id_cliente, id_repartidor, plataforma_origen, fecha, estado, total)
    assert(db:query(sql))
    peticion:respond(201, cjson.encode({ status = "ok", message = "Pedido creado" }))
end

function M.put(db, cjson, sanitizar, peticion, escape_sql)
    local body = cjson.decode(peticion.body or "{}")
    local id_pedido = tonumber(body.id_pedido)
    local id_cliente = tonumber(body.id_cliente)
    local id_repartidor = tonumber(body.id_repartidor)
    local total = tonumber(body.total)
    if not id_pedido or not id_cliente or not id_repartidor or not total then error("Parámetros numéricos inválidos") end

    local plataforma_origen = escape_sql(body.plataforma_origen)
    local fecha = escape_sql(body.fecha_hora)
    local estado = escape_sql(body.estado)

    local sql = string.format("UPDATE pedidos SET id_cliente = %d, id_repartidor = %d, plataforma_origen = '%s', fecha_hora = '%s', estado = '%s', total = %.2f WHERE id_pedido = %d;",
        id_cliente, id_repartidor, plataforma_origen, fecha, estado, total, id_pedido)
    assert(db:query(sql))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Pedido actualizado" }))
end

function M.delete(db, cjson, sanitizar, peticion, escape_sql)
    local raw_id = peticion.path:match("id_pedido=(%d+)")
    local id = tonumber(raw_id)
    if not id then error("Parámetro id_pedido faltante o inválido") end

    assert(db:query(string.format("DELETE FROM pedidos WHERE id_pedido = %d;", id)))
    peticion:respond(200, cjson.encode({ status = "ok", message = "Pedido eliminado" }))
end

return M