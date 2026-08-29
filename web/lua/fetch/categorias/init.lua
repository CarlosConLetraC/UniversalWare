local M = {}
function M.get(db, json, sanitizar, peticion)
    local res, err = db:query("SELECT * FROM categorias;")
    if not res then error(err) end
    peticion:respond(200, json.encode(sanitizar(res)))
end
return M