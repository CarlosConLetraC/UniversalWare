local Nodo = {}

function Nodo.new(valor, nodo)
    local nuevoNodo = {
        dato = valor,
        siguiente = nodo
    }
    return nuevoNodo
end

return Nodo