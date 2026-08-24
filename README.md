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