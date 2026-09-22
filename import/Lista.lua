local Lista = {}
local ListaMT = {__index = {}}

local io_write = io.write
local tostring = tostring

function ListaMT.__index.mostrar(self)
    if not self.cabeza then 
        io_write("Lista vacía\n")
        return 
    end
    local actual = self.cabeza
    while actual.siguiente do
        io_write(tostring(actual.dato) .. " -> ")
        actual = actual.siguiente
    end
    io_write(tostring(actual.dato) .. "\n")
end

function ListaMT.__index.insertarAlInicio(self, valor)
    self.cabeza = Nodo.new(valor, self.cabeza)
end

function ListaMT.__index.insertarAlFinal(self, valor)
    local nuevoNodo = Nodo.new(valor)
    if not self.cabeza then
        self.cabeza = nuevoNodo
        return self
    end

    local actual = self.cabeza
    while actual.siguiente do
        actual = actual.siguiente
    end
    
    actual.siguiente = nuevoNodo
    return self
end

function ListaMT.__index.insertarEnPosicion(self, valor, posicion)
    if posicion <= 1 or not self.cabeza then
        self:insertarAlInicio(valor)
        return
    end

    local nuevoNodo, actual = Nodo.new(valor), self.cabeza
    local i = 1

    while actual.siguiente and i < posicion - 1 do
        actual = actual.siguiente
        i = i + 1
    end

    nuevoNodo.siguiente = actual.siguiente
    actual.siguiente = nuevoNodo
end

function ListaMT.__index.buscar(self, valor)
    local actual = self.cabeza
    while actual do
        if actual.dato == valor then return true end
        actual = actual.siguiente
    end
    return false
end

function ListaMT.__index.recuperar(self, posicion)
    local actual = self.cabeza
    local i = 0

    while actual do
        if i == posicion then return actual end
        actual = actual.siguiente
        i = i + 1
    end
    return nil
end

function ListaMT.__index.modificar(self, posicion, nuevoValor)
    local nodoModificar = self:recuperar(posicion)
    if not nodoModificar then return false end
    nodoModificar.dato = nuevoValor
    return true
end

function ListaMT.__index.eliminar(self, valor)
    if not self.cabeza then return false end
    if self.cabeza.dato == valor then
        self.cabeza = self.cabeza.siguiente
        return true
    end

    local actual = self.cabeza
    while actual.siguiente and actual.siguiente.dato ~= valor do
        actual = actual.siguiente
    end

    if actual.siguiente then
        actual.siguiente = actual.siguiente.siguiente
        return true
    end
    return false
end

function Lista.new()
    local nuevaLista = { cabeza = nil }
    return setmetatable(nuevaLista, ListaMT)
end

return Lista