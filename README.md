# Airbnb Price Predictor: Documentación Técnica de Arquitectura

Este sistema es un motor de procesamiento concurrente de alto rendimiento diseñado para tareas de ciencia de datos intensivas. Implementa una arquitectura híbrida donde la eficiencia de **C++** se integra con la flexibilidad dinámica de **LuaJIT** y la capacidad analítica de **Python**.

## 1. Ecosistema de Herramientas de Shell e Interactividad

El proyecto abstrae la complejidad de las rutas y dependencias de LuaJIT mediante un conjunto de interfaces de línea de comandos (CLI) diseñadas para la depuración y la inspección de datos en tiempo real.

### 🖥 Interfaces de Ejecución

* **`./cmd` (Interface de Ejecución Directa)**:
    Permite ejecutar sentencias Lua aisladas directamente desde la terminal de Linux. Es ideal para inspeccionar el estado del sistema o probar funciones rápidas de las librerías cargadas.
    - *Ejemplo real*: `./cmd "system.print('Esto es un ejemplo', {{os}, 5, 10, 15})"`

* **`./initconsole` (Entorno REPL Interactivo)**:
    Lanza una consola interactiva (Read-Eval-Print Loop) con el núcleo del sistema (`import/init`) y librerías base (`Math`, `Table`, `system`) precargadas.
    - *Uso matemático*: `import('Vector3'); system.print(Vector3.one + Vector3.new(5,15,10))`
    - *Modo Scripting*: Soporta el flag `--exec` para ejecutar lógica compleja antes de entrar en modo interactivo.
    - *Ejemplo de inspección*: `./initconsole --exec "for k, v in Table.iter do system.print(k, v) end"`

* **`./runclient` (Lanzador de Aplicaciones)**:
    Punto de entrada para los scripts de producción (como `program.main.lua` o `merge.lua`). Gestiona la carga automática de plugins y el sistema de módulos antes de iniciar el proceso principal.

---

## 2. Infraestructura de Automatización y DevOps

El proyecto garantiza portabilidad absoluta y consistencia de entorno mediante contenedores y scripts de orquestación.

### 🐳 Gestión de Contenedores (`pods.sh`)
Script externo que define funciones para estandarizar el flujo de trabajo con **Podman**:
* `podmanbuild`: Construye la imagen asegurando que todas las dependencias (GCC, LuaJIT, Python) estén presentes.
* `podmanrun`: Ejecuta el contenedor con `--userns=keep-id` y montajes de volumen `:Z` para permitir persistencia de datos con los permisos de usuario del host.

### 🏗 Setup y Compilación
* **`configurarentorno.sh`**: Script de bootstrap que configura el entorno virtual de Python (`entorno/`), instala dependencias de sistema y prepara el gestor de paquetes `luarocks`.
* **`build.sh`**: Punto de entrada para la compilación del binario `backend` en C++17, optimizado para el hardware local.

---

## 3. Pipeline de Procesamiento de Datos (`run.sh`)

El archivo `run.sh` coordina el flujo completo de ciencia de datos de forma secuencial:

1.  **Generación de Jobs**: `runclient program.main.lua` particiona el dataset original `data/train.csv` en múltiples fragmentos (`dataset_train_*.csv`) y crea la cola en `daemons/`.
2.  **Procesamiento Concurrente (`backend`)**: El binario de C++ distribuye los shards entre hilos paralelos. Cada hilo ejecuta un worker LuaJIT para limpieza, normalización y **One-Hot Encoding**.
3.  **Consolidación**: `runclient merge.lua` unifica los resultados procesados en un único set de datos.
4.  **Entrenamiento**: El motor C++ calcula la Regresión Lineal Múltiple y exporta los pesos a `data/model_final.json`.
5.  **Visualización**: Se ejecuta **`el_de_los_mandados.py`** para generar las gráficas de diagnóstico en el directorio `/plots`.

---

## 4. Resultados y Métricas del Modelo

Basado en el procesamiento de **74,113 registros**, el sistema reporta:

| Métrica | Valor |
| :--- | :--- |
| **$R^2$ (Coeficiente de Determinación)** | **0.5281** |
| **MSE (Error Cuadrático Medio)** | **0.2439** |
| **MAE (Error Absoluto Medio)** | **0.3720** |

### Hallazgos de Análisis
* **Impacto de Privacidad**: La variable `room_shared` es el predictor con el impacto negativo más fuerte (**-0.742**).
* **Capacidad Física**: El número de habitaciones (`bedrooms`: +0.146) y baños (`bathrooms`: +0.133) muestran una correlación positiva directa con el precio.

---

## 🛠 Instrucciones de Uso (Flujo Completo)

1. **Cargar funciones**: `source pods.sh`
2. **Construir entorno**: `podmanbuild Dockerfile airbnb-proj`
3. **Ejecutar contenedor**: `podmanrun airbnb-proj`
4. **Ejecutar experimento**: `./run.sh`

---
## Instalación del software
```bash
git clone --recursive "https://github.com/CarlosConLetraC/ProyectoFinal_CienciaDeDatos.git"
cd ProyectoFinal_CienciaDeDatos
chmod +x initconsole cmd runclient *.sh
```
---

## Sistemas compatibles

- Debian 13 (Trixie)
- Ubuntu 22.04 (Jammy)

---

# Configurar el entorno (recomendado usar podman)
```bash
./configurarentorno.sh
```
---

# Compilar backend.cpp
```bash
./build.sh
```
---

# Ejecutar proyecto completo
```bash
./run.sh
```

---