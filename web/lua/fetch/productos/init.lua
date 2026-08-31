local M = {}

-- 1. GET: Listar o consultar registros
function M.get(db, json, sanitizar, peticion)
    local query = "SELECT * FROM productos;" -- Reemplaza con tu tabla
    local res, err = db:query(query)
    
    if not res then
        peticion:respond(500, json.encode({ error = "Error ejecutando query", detalle = tostring(err) }))
        return
    end

    peticion:respond(200, json.encode(sanitizar(res)))
end

-- 2. POST: Insertar un nuevo registro
function M.post(db, json, sanitizar, peticion)
    local body_data = json.decode(peticion.body)
    if not body_data then
        peticion:respond(400, json.encode({ error = "Cuerpo JSON inválido o vacío" }))
        return
    end

    -- Construcción de la consulta utilizando los datos del JSON recibido
    -- Asegúrate de sanitizar o formatear los valores según corresponda a tu esquema
    local query = string.format(
        "INSERT INTO productos (id_marca, id_categoria, nombre, precio_venta) VALUES (%s, %s, '%s', %s)",
        tostring(body_data.id_marca),
        tostring(body_data.id_categoria),
        tostring(body_data.nombre),
        tostring(body_data.precio_venta)
    )

    local res, err = db:query(query)
    if not res then
        peticion:respond(500, json.encode({ error = "Error al insertar en la base de datos", detalle = tostring(err) }))
        return
    end

    peticion:respond(201, json.encode({ mensaje = "Registro creado exitosamente" }))
end

-- 3. PUT: Actualizar un registro existente
function M.put(db, json, sanitizar, peticion)
    local body_data = json.decode(peticion.body)
    if not body_data or not body_data.id_producto then
        peticion:respond(400, json.encode({ error = "Faltan datos o el ID para actualizar" }))
        return
    end

    local query = string.format(
        "UPDATE productos SET nombre = '%s', precio_venta = %s WHERE id_producto = %s",
        tostring(body_data.nombre),
        tostring(body_data.precio_venta),
        tostring(body_data.id_producto)
    )

    local res, err = db:query(query)
    if not res then
        peticion:respond(500, json.encode({ error = "Error al actualizar", detalle = tostring(err) }))
        return
    end

    peticion:respond(200, json.encode({ mensaje = "Registro actualizado exitosamente" }))
end

-- 4. DELETE: Eliminar un registro
function M.delete(db, json, sanitizar, peticion)
    local body_data = json.decode(peticion.body)
    if not body_data or not body_data.id_producto then
        peticion:respond(400, json.encode({ error = "Se requiere el ID para eliminar" }))
        return
    end

    local query = string.format(
        "DELETE FROM productos WHERE id_producto = %s",
        tostring(body_data.id_producto)
    )

    local res, err = db:query(query)
    if not res then
        peticion:respond(500, json.encode({ error = "Error al eliminar el registro", detalle = tostring(err) }))
        return
    end

    peticion:respond(200, json.encode({ mensaje = "Registro eliminado exitosamente" }))
end

return M