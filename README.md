# 🚀 UniversalWare

> **UniversalWare** es un ecosistema de software híbrido de alto rendimiento diseñado para orquestar tareas concurrentes con baja latencia y control estricto de recursos. Combina la velocidad de C++17 y librerías nativas con la agilidad de LuaJIT para la lógica de negocio.

---

## 📋 Tabla de Contenidos
- [Vista General](#-vista-general)
- [Caso de Uso: Dark Kitchen](#-caso-de-uso-dark-kitchen)
- [Arquitectura del Sistema](#-arquitectura-del-sistema)
- [Estructura del Repositorio](#-estructura-del-repositorio)
- [Requisitos del Sistema](#-requisitos-del-sistema)
- [Instalación y Uso](#-instalación-y-uso)
- [Herramientas CLI y Diagnóstico](#-herramientas-cli-y-diagnóstico)
- [Despliegue con Contenedores (Podman / Docker)](#-despliegue-con-contenedores-podman--docker)
- [Licencia](#-licencia)

---

## 📸 Vista General

El sistema está diseñado bajo el principio de **separación de responsabilidades**:
- **C++17 (`backend.cpp` + `libbackend`)**: Maneja la orquestación multihilo, balanceo de carga, límites de recursos (CPU/RAM) y la interfaz de bajo nivel con MariaDB.
- **LuaJIT (`program.*.lua`)**: Proporciona un entorno ágil e hiper-rápido para implementar reglas de negocio sin necesidad de recompilar el núcleo.
- **Shell Automation (`run.sh`, `pods.sh`, `cmd`)**: Ofrece herramientas idóneas para CI/CD local, entorno interactivo REPL y compilación aislada.

---

## 🍕 Caso de Uso: Dark Kitchen

El repositorio incluye un caso de estudio enfocado en la gestión integral de una **Dark Kitchen** (cocina fantasma de alto volumen):
* **Ingesta de Pedidos en Tiempo Real:** Persistencia continua en MariaDB mediante el driver nativo `cmariadb` con manejo de datos mediante `SqlValue`.
* **Cálculo de Rutas Logísticas:** Utilización del módulo nativo `cgraph` (Dijkstra) para la asignación eficiente de repartidores.
* **Control de Concurrencia:** Orquestación de múltiples instancias Lua paralelas sin colapsar el hardware ni generar cuellos de botella en la toma de comanda.

---

## 🏗️ Arquitectura del Sistema

```text
 ┌────────────────────────────────────────────────────────────────────────┐
 │                      CAPA DE ORQUESTACIÓN Y CLI                        │
 │  ┌──────────────┐    ┌─────────────┐    ┌──────────────┐  ┌─────────┐  │
 │  │    run.sh    │    │ backend.cpp │    │ initconsole  │  │   cmd   │  │
 │  └──────┬───────┘    └──────┬──────┘    └──────┬───────┘  └────┬────┘  │
 └─────────┼───────────────────┼──────────────────┼───────────────┼───────┘
           │                   │                  │               │
 ┌─────────▼───────────────────▼──────────────────▼───────────────▼───────┐
 │                      CAPA DE EJECUCIÓN (RUNTIME)                       │
 │  ┌──────────────────────────────────────────────────────────────────┐  │
 │  │                         runclient / LuaJIT                       │  │
 │  └──────────────────────────────────┬───────────────────────────────┘  │
 │                                     │                                  │
 │                         ┌───────────▼───────────┐                      │
 │                         │   program.main.lua    │                      │
 │                         └───────────┬───────────┘                      │
 └─────────────────────────────────────┼──────────────────────────────────┘
                                       │
 ┌─────────────────────────────────────▼──────────────────────────────────┐
 │                      CAPA DE NÚCLEO NATIVO (C/C++)                     │
 │   ┌───────────────────────┐   ┌─────────────────┐   ┌──────────────┐   │
 │   │   libbackend / cjob   │   │    cmariadb     │   │   cgraph     │   │
 │   │ (ThreadPool, Scheduler│   │ (Driver C++ SQL)│   │  (Dijkstra)  │   │
 │   └───────────────────────┘   └────────┬────────┘   └──────────────┘   │
 └────────────────────────────────────────┼───────────────────────────────┘
                                          │
 ┌────────────────────────────────────────▼───────────────────────────────┐
 │                           CAPA DE PERSISTENCIA                         │
 │                         ┌─────────────────────┐                        │
 │                         │  MariaDB / MySQL    │                        │
 │                         └─────────────────────┘                        │
 └────────────────────────────────────────────────────────────────────────┘
```

---

## 📂 Estructura del Repositorio

```text
UniversalWare/
├── backend.cpp               # Orquestador principal en C++17
├── build.sh                  # Script de compilación del proyecto
├── clibs/                    # Librerías nativas en C (cmariadb, cjob, etc.)
├── cmd                       # CLI para ejecutar sentencias Lua rápidas
├── configurarentorno.sh      # Script de preparación de dependencias y venv
├── cpplibs/                  # Librerías dinámicas C++ (cgraph, etc.)
├── import/                   # Módulos compartidos e inicializadores de Lua
├── initconsole               # Consola REPL interactiva de diagnósticos
├── libbackend/               # Motor de multihilo Header-Only (Scheduler, ThreadPool)
│   ├── Broker.h
│   ├── Job.h
│   ├── Scheduler.h
│   ├── ThreadPool.h
│   └── Worker.h
├── pods.sh                   # Funciones Bash helper para construir/ejecutar Podman
├── program.main.lua          # Punto de entrada principal en Lua
├── run.sh                    # Entrypoint unificado con auto-recuperación
├── runclient                 # Wrapper de ejecución de entornos para LuaJIT
└── ubuntu_jammy.dockerfile   # Dockerfile con usuario no-root 'pc' (Ubuntu 22.04)
```

---

## ⚙️ Requisitos del Sistema

- **Sistemas Operativos:** Linux (probado en CachyOS / Arch Linux, Ubuntu 22.04 LTS).
- **Compilador:** GCC / G++ con soporte completo para C++17.
- **Intérprete:** LuaJIT 2.1+.
- **Base de Datos:** MariaDB / MySQL Server (`libmariadb-dev`).
- **Python:** Python 3.x (`python3-venv`).

---

## 🚀 Instalación y Uso

### 1. Clonar el repositorio
```bash
git clone https://github.com/tu-usuario/UniversalWare.git
cd UniversalWare
```

### 2. Ejecución Automatizada (Recomendado)
El script `run.sh` es **idempotente y tolerante a fallos**. Detecta la falta de binarios, configura el entorno virtual y compila el orquestador si es necesario antes de arrancar:

```bash
chmod +x run.sh
./run.sh
```

### 3. Compilación Manual de `backend`
Si deseas compilar únicamente el orquestador C++17 con optimizaciones `-O3`:

```bash
g++ -std=c++17 -O3 backend.cpp -I. -lpthread -o backend
```

Para ejecutarlo pasando un directorio con instancias Lua (`program*.lua`):

```bash
./backend instanciasUniversalWare/
```

---

## 🛠️ Herramientas CLI y Diagnóstico

### Consola Interactiva REPL (`initconsole`)
Abre una sesión activa de LuaJIT cargando el entorno de `import/`:
```bash
./initconsole
```

### Ejecución de Comandos Directos (`cmd`)
Permite evaluar una instrucción de Lua en una sola línea manteniendo las dependencias cargadas:
```bash
./cmd "print('Estado del sistema OK')"
```

---

## 🐳 Despliegue con Contenedores (Podman / Docker)

El proyecto cuenta con integración nativa para **Podman** a través de `ubuntu_jammy.dockerfile` (configurado con un usuario no-root `pc` en `/home/pc`) y las funciones facilitadoras en `pods.sh`:

### 1. Cargar las funciones helper en la sesión de terminal
```bash
source pods.sh
```

### 2. Compilar la imagen del contenedor
```bash
podmanbuild ubuntu_jammy.dockerfile universalware-env
```

### 3. Ejecutar el contenedor con montaje del directorio actual
La función `podmanrun` mapea el directorio del proyecto a `/home/pc` dentro del contenedor manteniendo permisos con `--userns=keep-id`:

```bash
podmanrun universalware-env
```

Una vez dentro del contenedor, puedes iniciar el ecosistema mediante:
```bash
./run.sh
```

---

## 📄 Licencia

Este proyecto está bajo la Licencia **MIT** (o la licencia especificada en el archivo [LICENCE](LICENCE)). Sientete libre de consultar el archivo para más detalles.