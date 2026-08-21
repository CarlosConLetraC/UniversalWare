local Lista = {}
local ListaMT = {__index = {}}

local io_write = io.write
local tostring, pairs = tostring, pairs

function ListaMT.__index.mostrar(self)
	if not self.cabeza then return end
	local actual = self.cabeza
	while actual.siguiente do
		io_write(tostring(actual.dato)..(actual.siguiente and "->") or "\n")
		actual = actual.siguiente
	end
	io_write(tostring(actual.dato)..'\n')
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
	if not self.cabeza then return end
	local nuevoNodo, actual = Nodo.new(valor), self.cabeza

	for i = 1, posicion, 1 do
		if i ~= posicion and actual.siguiente then
			actual = actual.siguiente
		else
			break
		end
	end

	nuevoNodo.siguiente = actual.siguiente
	actual.siguiente = nuevoNodo
end

function ListaMT.__index.buscar(self, valor)
	if not self.cabeza then return false end
	for _, actual in pairs(self) do
		if actual.dato == valor then return true end
	end
	return false
end

function ListaMT.__index.recuperar(self, posicion)
	if not self.cabeza then return end
	local actual = self.cabeza
	local i = 0

	while actual do
		if i == posicion then return actual end
		actual = actual.siguiente
		i = i + 1
	end
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

function Lista.new(...)
	local nuevaLista = {...}
	return setmetatable(nuevaLista, ListaMT)
end

return Lista