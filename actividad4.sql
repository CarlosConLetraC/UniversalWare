DROP DATABASE IF EXISTS zoo;
CREATE DATABASE IF NOT EXISTS zoo;
USE zoo;

CREATE TABLE IF NOT EXISTS animales (
    id_animal INT AUTO_INCREMENT PRIMARY KEY,
    nombre_animal VARCHAR(150) NOT NULL,
    tipo_animal ENUM('terrestre', 'aereo', 'acuatico'),
    propiedades_animal JSON
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS horarios (
    id_horarios INT AUTO_INCREMENT PRIMARY KEY,
    dias_apertura_horarios ENUM('lunes', 'martes', 'miercoles', 'jueves', 'viernes', 'sabado'),
    apertura_horarios TIME,
    cierre_horarios TIME
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS sucursal (
    id_sucursal INT AUTO_INCREMENT PRIMARY KEY,
    nombre_sucursal VARCHAR(150) NOT NULL,
    direccion_sucursal JSON,
    id_horarios_sucursal INT NOT NULL,
    FOREIGN KEY (id_horarios_sucursal) REFERENCES horarios(id_horarios) ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS empleados (
    id_empleado INT AUTO_INCREMENT PRIMARY KEY,
    fecha_registro DATE,
    nombre_empleado VARCHAR(150),
    edad_empleado TINYINT NOT NULL,
    sexo_empleado ENUM('F', 'M'),
    salario_empleado FLOAT NOT NULL,
    id_sucursal INT NOT NULL,
    FOREIGN KEY (id_sucursal) REFERENCES sucursal(id_sucursal) ON DELETE RESTRICT ON UPDATE CASCADE,
    id_horarios_empleados INT NOT NULL,
    FOREIGN KEY (id_horarios_empleados) REFERENCES horarios(id_horarios) ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS membresias(
    id_membresia INT AUTO_INCREMENT PRIMARY KEY,
    tipo_membresia ENUM('basico', 'premium', 'vip'),
    detalles_membresia JSON
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS clientes (
    id_cliente INT AUTO_INCREMENT PRIMARY KEY,
    nombre_cliente VARCHAR(150) NOT NULL,
    sexo_cliente ENUM('F', 'M'),
    edad_cliente TINYINT,
    FOREIGN KEY (id_cliente) REFERENCES membresias(id_membresia) ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS productos (
    id_producto INT AUTO_INCREMENT PRIMARY KEY,
    nombre_producto VARCHAR(150) NOT NULL,
    tipo_producto ENUM('alimento', 'recuerdo'),
    detalles_producto JSON,
    stock_producto INT UNSIGNED DEFAULT 0,
    existencia_producto BOOLEAN DEFAULT false
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS ventas (
    id_venta INT AUTO_INCREMENT PRIMARY KEY,
    monto_venta FLOAT NOT NULL,
    id_cliente INT NOT NULL,
    id_empleado INT NOT NULL,
    id_sucursal INT NOT NULL,
    FOREIGN KEY (id_cliente) REFERENCES clientes(id_cliente) ON DELETE RESTRICT ON UPDATE CASCADE,
    FOREIGN KEY (id_empleado) REFERENCES empleados(id_empleado) ON DELETE RESTRICT ON UPDATE CASCADE,
    FOREIGN KEY (id_sucursal) REFERENCES sucursal(id_sucursal) ON DELETE RESTRICT ON UPDATE CASCADE,
    fecha_venta DATETIME
) ENGINE=InnoDB;

INSERT INTO animales(nombre_animal, tipo_animal, propiedades_animal) VALUES ("Mango", 'terrestre', '{"taxonomia": "gato"}');

SELECT * FROM animales;

DROP DATABASE IF EXISTS zoo;