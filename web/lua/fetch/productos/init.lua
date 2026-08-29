local M = {}

function M.get(db, json, sanitizar, peticion)
    print("[DEBUG] Ejecutando consulta en productos...")
    local query = "SELECT p.id_producto AS id_producto, p.nombre AS producto, m.nombre AS marca, p.precio_venta AS precio_venta FROM productos p JOIN marcas m ON p.id_marca = m.id_marca;"
    
    local res, err = db:query(query)
    if not res then error(err) end
    
    print("[DEBUG] Consulta exitosa, sanitizando datos...")
    local datos = sanitizar(res)
    local json_str = json.encode(datos)
    
    print("[DEBUG] Respondiendo al cliente HTTP...")
    
    -- Intenta enviar cabeceras explícitas si tu función chttp lo soporta, 
    -- o prueba si peticion:respond acepta un formato de tabla/headers:
    if peticion.send then
        peticion:send(200, "Content-Type: application/json\r\n\r\n" .. json_str)
    else
        peticion:respond(200, json_str)
    end
    print("[DEBUG] ¡Respuesta enviada con éxito!")
end

return M