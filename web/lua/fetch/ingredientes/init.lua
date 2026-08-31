local M = {}

function M.get(db, json, sanitizar, peticion)
    local query = "SELECT * FROM ingredientes;"
    local res, err = db:query(query)
    
    if not res then
        peticion:respond(500, json.encode({ error = "Error ejecutando query", detalle = tostring(err) }))
        return
    end

    peticion:respond(200, json.encode(sanitizar(res)))
end

return M