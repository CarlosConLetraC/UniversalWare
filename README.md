# 🚀 UniversalWare

> **UniversalWare** es un ecosistema de software híbrido de alto rendimiento diseñado para orquestar tareas concurrentes con baja latencia y control estricto de recursos. Combina la velocidad de C/C++ y librerías dinámicas (`.so`) con la agilidad de LuaJIT para la lógica de negocio.

---

## 📋 Tabla de Contenidos
- [Vista General](#-vista-general)
- [Caso de Uso: Dark Kitchen](#-caso-de-uso-dark-kitchen)
- [Arquitectura del Sistema](#-arquitectura-del-sistema)
- [Estructura Completa del Repositorio](#-estructura-completa-del-repositorio)
- [Requisitos del Sistema](#-requisitos-del-sistema)
- [Instalación y Uso](#-instalación-y-uso)
- [Herramientas CLI y Diagnóstico](#-herramientas-cli-y-diagnóstico)
- [Despliegue con Contenedores (Podman / Docker)](#-despliegue-con-contenedores-podman--docker)
- [Licencia](#-licencia)

---

## 📸 Vista General

El sistema está diseñado bajo el principio de **separación de responsabilidades**:
- **C++17 (`backend.cpp` + `libbackend/`)**: Orquestador principal multihilo que gestiona el pool de trabajadores (`Worker.h`), la cola de trabajos (`Job.h`, `Scheduler.h`) y la distribución (`Broker.h`).
- **Librerías Nativas y Dinámicas (`clibs/`, `cpplibs/`, `import/Linux/`)**: Módulos optimizados para MariaDB (`cmariadb.so`), multitarea/planificación (`cjob.so`), utilidades estadísticas (`cstats.so`), procesamiento CSV acelerado (`csvfast.so`), Machine Learning (`cml.so`) y utilidades SSH (`ssh.so`).
- **Módulos y Utilidades Lua (`import/`)**: Proporcionan abstracciones matemáticas/vectoriales (`Vector2`, `Vector3`, `Color3`, `Math`), compatibilidad con formatos (`json/cjson`, `csv/csvfast`, `base64`), enums visuales/animación (`EasingModes`).
- **LuaJIT (`program.main.lua`)**: Punto de entrada de alto nivel para ejecutar reglas de negocio e iteraciones dinámicas sin recompensar el núcleo.
- **Shell Automation (`run.sh`, `pods.sh`, `cmd`, `build.sh`)**: Automatización completa para CI/CD local, entorno interactivo REPL, compilación y despliegue.

---

## 🍕 Caso de Uso: Dark Kitchen

El repositorio incluye un caso de estudio enfocado en la gestión integral de una **Dark Kitchen** (cocina fantasma de alto volumen):
* **Ingesta de Pedidos en Tiempo Real:** Persistencia continua y lectura transaccional en MariaDB a través del módulo nativo `cmariadb.so`.
* **Carga Masiva de Menús e Inventarios:** Procesamiento e ingesta ultrarrápida de archivos CSV de insumos mediante `csvfast.so` y `csv.lua`.
* **Analítica y Proyecciones de Demanda:** De ser necesario, se puede incluir operaciones de modelado estocástico de pedidos e inventarios críticos combinando el rendimiento de `cstats.so` y `cml.so` para análisis de datos.
* **Orquestación Concurrente:** Procesamiento de comandas y ejecución de tareas pesadas en segundo plano mediante `cjob.so` y el motor multihilo de `libbackend/`.

---

## 🏗️ Arquitectura del Sistema

```mermaid
flowchart TD

subgraph group_orchestration["Orquestación concurrente"]
  node_backend["Orquestador C++<br/>[backend.cpp]"]
  node_scheduler["Scheduler multihilo<br/>[Scheduler.h]"]
  node_job_queue["Cola de trabajos<br/>[Job.h]"]
  node_worker["Worker ejecutor<br/>[Worker.h]"]
end

subgraph group_runtime_web["Runtime y API"]
  node_lua_runtime["Runtime LuaJIT"]
  node_business_scripts["Programas Lua<br/>[program.main.lua]"]
  node_http_api["API dark kitchen<br/>[program.fetch.lua]"]
  node_crud_handlers["Handlers CRUD"]
  node_http_transport["Servidor HTTP<br/>[server.c]"]
end

subgraph group_native_processing["Procesamiento nativo"]
  node_mariadb_module["Cliente MariaDB<br/>[cmariadb/main.c]"]
  node_csv_engine["Procesador CSV<br/>[csvfast.cpp]"]
  node_stats_engine["Estadística nativa<br/>[cstats.c]"]
  node_ml_engine["Modelos ML<br/>[cml.cpp]"]
  node_json_bridge["Puente JSON<br/>[cjson.c]"]
  node_native_jobs["Tareas nativas<br/>[cjob/main.c]"]
end

subgraph group_persistence["Persistencia y datos"]
  node_mariadb[("MariaDB")]
  node_data_files["Datos JSON y CSV"]
end

node_browser(("Navegador usuario"))

node_browser -->|"solicita API"| node_http_api
node_browser -->|"abre conexión"| node_http_transport
node_http_api -->|"usa HTTP"| node_http_transport
node_http_transport -->|"entrega solicitudes"| node_http_api
node_http_api -->|"despacha rutas"| node_crud_handlers
node_crud_handlers -->|"consulta datos"| node_mariadb_module
node_mariadb_module -->|"lee y escribe"| node_mariadb
node_http_api -->|"serializa JSON"| node_json_bridge
node_backend -->|"inicia scheduler"| node_scheduler
node_backend -->|"crea trabajos"| node_job_queue
node_scheduler -->|"despacha trabajos"| node_worker
node_worker -->|"lanza LuaJIT"| node_lua_runtime
node_lua_runtime -->|"ejecuta programas"| node_business_scripts
node_lua_runtime -->|"ejecuta servidor"| node_http_api
node_business_scripts -->|"procesa CSV"| node_csv_engine
node_csv_engine -->|"lee y escribe"| node_data_files
node_business_scripts -->|"calcula métricas"| node_stats_engine
node_business_scripts -->|"entrena modelos"| node_ml_engine
node_business_scripts -->|"codifica datos"| node_json_bridge
node_business_scripts -->|"lee configuración"| node_data_files
node_business_scripts -.->|"programa tareas"| node_native_jobs

click node_backend "https://github.com/carlosconletrac/universalware/blob/main/backend.cpp"
click node_scheduler "https://github.com/carlosconletrac/universalware/blob/main/libbackend/Scheduler.h"
click node_job_queue "https://github.com/carlosconletrac/universalware/blob/main/libbackend/Job.h"
click node_worker "https://github.com/carlosconletrac/universalware/blob/main/libbackend/Worker.h"
click node_lua_runtime "https://github.com/carlosconletrac/universalware/blob/main/runclient"
click node_business_scripts "https://github.com/carlosconletrac/universalware/blob/main/program.main.lua"
click node_http_api "https://github.com/carlosconletrac/universalware/blob/main/web/lua/program.fetch.lua"
click node_crud_handlers "https://github.com/carlosconletrac/universalware/tree/main/web/lua/fetch"
click node_http_transport "https://github.com/carlosconletrac/universalware/blob/main/clibs/chttp/server.c"
click node_mariadb_module "https://github.com/carlosconletrac/universalware/blob/main/clibs/cmariadb/main.c"
click node_csv_engine "https://github.com/carlosconletrac/universalware/blob/main/cpplibs/csvfast.cpp"
click node_stats_engine "https://github.com/carlosconletrac/universalware/blob/main/clibs/cstats.c"
click node_ml_engine "https://github.com/carlosconletrac/universalware/blob/main/cpplibs/cml.cpp"
click node_json_bridge "https://github.com/carlosconletrac/universalware/blob/main/clibs/cjson.c"
click node_native_jobs "https://github.com/carlosconletrac/universalware/blob/main/clibs/cjob/main.c"
click node_data_files "https://github.com/carlosconletrac/universalware/tree/main/data"

classDef toneNeutral fill:#f8fafc,stroke:#334155,stroke-width:1.5px,color:#0f172a
classDef toneBlue fill:#dbeafe,stroke:#2563eb,stroke-width:1.5px,color:#172554
classDef toneAmber fill:#fef3c7,stroke:#d97706,stroke-width:1.5px,color:#78350f
classDef toneMint fill:#dcfce7,stroke:#16a34a,stroke-width:1.5px,color:#14532d
classDef toneRose fill:#ffe4e6,stroke:#e11d48,stroke-width:1.5px,color:#881337
classDef toneIndigo fill:#e0e7ff,stroke:#4f46e5,stroke-width:1.5px,color:#312e81
classDef toneTeal fill:#ccfbf1,stroke:#0f766e,stroke-width:1.5px,color:#134e4a
class node_backend,node_scheduler,node_job_queue,node_worker toneBlue
class node_lua_runtime,node_business_scripts,node_http_api,node_crud_handlers,node_http_transport toneAmber
class node_mariadb_module,node_csv_engine,node_stats_engine,node_ml_engine,node_json_bridge,node_native_jobs toneMint
class node_mariadb,node_data_files toneRose
class node_browser toneIndigo
```

## 📂 Estructura Completa del Repositorio

```text
UniversalWare/
├── backend                   # Binario ejecutable compilado del orquestador
├── backend.cpp               # Orquestador principal en C++17
├── build.sh                  # Script de compilación de módulos C/C++ y librerías dinámicas
├── clibs/                    # Código fuente C de librerías nativas
│   ├── cjob/                 # Gestor/programador nativo de tareas (cjob.h, job.c, scheduler.c)
│   ├── cmariadb/             # Driver cliente MariaDB (clientmodes.h, datatypes.h, datatypes.c, main.c)
│   └── cstats.c              # Módulo C de cálculo estadístico
├── cmd                       # CLI para evaluar sentencias Lua directas
├── configurarentorno.sh      # Preparación de dependencias y venv
├── cpplibs/                  # Código fuente C++ de librerías dinámicas
│   ├── cml.cpp               # Módulo C++ para Machine Learning
│   └── csvfast.cpp           # Parser optimizado de archivos CSV
├── data/                     # Archivos de configuración y semillas de datos JSON
│   ├── guid_rand.json
│   └── rand_config.json
├── import/                   # Módulos, librerías y bins binarios para Lua
│   ├── base64.lua / bit32.lua
│   ├── Color3.lua / File.lua / Math.lua / String.lua / Table.lua
│   ├── json.lua / system.lua
│   ├── Lista.lua / Nodo.lua
│   ├── EasingModes.lua / Enum/ (EaseMode, Faces, FaceType, Shape, StyleMode)
│   ├── sublibs/               # Tipos vectoriales secundarios (Color3, Vector2, Vector3)
│   ├── Linux/                # Binarios dinámicos compilados (*.so) para Linux
│   │   └── nada.lua
│   └── Windows/              # Mapeo de compatibilidad para plataformas Windows
│       └── nada.lua
├── initconsole               # Consola REPL interactiva con entorno pre-cargado
├── libbackend/               # Motor multihilo Header-Only en C++
│   ├── Broker.h
│   ├── Job.h
│   ├── Scheduler.h
│   ├── ThreadPool.h
│   └── Worker.h
├── LICENCE                   # Licencia del proyecto
├── pods.sh                   # Helpers Bash para construcción y ejecución con Podman
├── program.main.lua          # Punto de entrada principal en Lua
├── README.md                 # Documentación del proyecto
├── runclient                 # Wrapper de ejecución del runtime LuaJIT
├── run.sh                    # Entrypoint principal de automatización
└── ubuntu_jammy.dockerfile   # Dockerfile no-root (usuario 'pc') basado en Ubuntu 22.04
```

---

## ⚙️ Requisitos del Sistema

- **Sistemas Operativos:** Linux (probado y optimizado en CachyOS / Arch Linux, compatible con Ubuntu 22.04 LTS).
- **Contenedores:** *Podman* (requerido para la compilación aislada y construcción del entorno/backend).
- **Compilador:** GCC / G++ con soporte completo para C++17.
- **Intérprete:** LuaJIT 2.1.178+.
- **Base de Datos:** MariaDB / MySQL Server (`mariadb-libs` en Arch/CachyOS o `libmariadb-dev` en Debian/Ubuntu).

---

## 🚀 Instalación y Uso

### 1. Clonar el repositorio
```bash
git clone https://github.com/tu-usuario/UniversalWare.git
cd UniversalWare
```

### 2. Ejecución Automatizada (Recomendado)
El script `run.sh` es **idempotente y tolerante a fallos**. Verificará dependencias, compilará componentes faltantes y ejecutará el ecosistema:

```bash
chmod +x run.sh
./run.sh
```

### 3. Compilación Manual de Módulos y `backend`
Para reconstruir los binarios nativos `.so` y el ejecutable principal:

```bash
chmod +x build.sh
./build.sh
```

---

## 🛠️ Herramientas CLI y Diagnóstico

### Consola Interactiva REPL (`initconsole`)
Abre una sesión activa de LuaJIT cargando automáticamente el entorno en `import/`:
```bash
./initconsole
```

### Ejecución de Comandos Directos (`cmd`)
Evalúa expresiones o archivos Lua manteniendo el contexto precargado:
```bash
./cmd "print('UniversalWare listo')"
```

---

## 🐳 Despliegue con Contenedores (Podman / Docker)

El proyecto incluye integración completa con **Podman** usando `ubuntu_jammy.dockerfile` (configurado con el usuario `pc` en `/home/pc`) y las funciones facilitadoras en `pods.sh`:

### 1. Cargar las funciones helper
```bash
source pods.sh
```

### 2. Compilar la imagen del contenedor
```bash
podmanbuild ubuntu_jammy.dockerfile universalware-env
```

### 3. Ejecutar el contenedor
La función `podmanrun` mapea el proyecto a `/home/pc` con el flag `--userns=keep-id`:

```bash
podmanrun universalware-env
```

Una vez dentro del contenedor:
```bash
cd RUTA_DE_UNIVERSAL_WARE
./configurarentorno.sh
```

Opcionalmente, puede iniciar el programa dentro del contenedor o en el sistema principal.
```bash
./run.sh
```
---

## 🎥 Demostración del Sistema

<img width="1280" height="800" alt="UniversalWare_Ejemplo1" src="https://github.com/user-attachments/assets/74b1c133-6e8d-46d4-8fbe-52ee821899e0" />

---

## 📄 Licencia

Este proyecto está bajo la Licencia **MIT** (o la especificada en el archivo [LICENCE](LICENCE)).
