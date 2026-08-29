import("cmariadb", "cjob", "chttp", "json")

print("[DEBUG] Cargando program.fetch.lua correctamente...")

cjob.new(function()
    print("[DEBUG] Entrando a la corrutina principal...")
    chttp.listen("127.0.0.1", 8081)
    print("[INFO] Servidor HTTP escuchando en el puerto 8081.")

    while true do
        print("[DEBUG] Esperando petición (chttp.accept)...")
        local peticion = chttp.accept()
        if peticion then
            print("[DEBUG] ¡Petición recibida!", peticion.method, peticion.path)
            -- (El resto de tu lógica de controladores...)
        else
            print("[DEBUG] chttp.accept() devolvió nil o bloqueó.")
        end
    end
end)

print("[DEBUG] Llamando a cjob.async()...")
cjob.async()