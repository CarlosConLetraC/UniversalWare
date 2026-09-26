/* * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                 *
 * Firma SQL tipo DML (Data Manipulation Language) *
 *                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * NOTA: cada %s es un operador de formato usado en la interfaz de LuaJIT.
 * Si ejecuta esto tal cual tendrá problemas. Considere usar program.main.lua en su lugar.
 */
SET FOREIGN_KEY_CHECKS = 0;

INSERT INTO marcas (nombre, activa) VALUES %s
ON DUPLICATE KEY UPDATE nombre=VALUES(nombre);

INSERT INTO categorias (nombre) VALUES %s
ON DUPLICATE KEY UPDATE nombre=VALUES(nombre);

INSERT INTO productos (id_marca, id_categoria, nombre, precio_venta) VALUES %s
ON DUPLICATE KEY UPDATE precio_venta=VALUES(precio_venta);

INSERT INTO ingredientes (nombre, unidad_medida, stock_actual, stock_minimo) VALUES %s
ON DUPLICATE KEY UPDATE stock_actual=VALUES(stock_actual);

INSERT INTO recetas (id_producto, id_ingrediente, cantidad_requerida) VALUES %s
ON DUPLICATE KEY UPDATE cantidad_requerida=VALUES(cantidad_requerida);

INSERT INTO clientes (nombre, telefono, direccion) VALUES %s
ON DUPLICATE KEY UPDATE telefono=VALUES(telefono);

INSERT INTO repartidores (nombre, telefono, vehiculo) VALUES %s
ON DUPLICATE KEY UPDATE vehiculo=VALUES(vehiculo);

INSERT INTO pedidos (id_cliente, id_repartidor, plataforma_origen, fecha_hora, estado, total) VALUES %s
ON DUPLICATE KEY UPDATE estado=VALUES(estado);

INSERT INTO detalle_pedidos (id_pedido, id_producto, cantidad, precio_unitario) VALUES %s
ON DUPLICATE KEY UPDATE cantidad=VALUES(cantidad);

SET FOREIGN_KEY_CHECKS = 1;