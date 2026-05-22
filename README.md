# Organización y Arquitectura de Computadoras

Este repositorio contiene los laboratorios y prácticas del curso de Organización y Arquitectura de Computadoras.

## Estructura del Repositorio

- **plantillas/** - Plantilla LaTeX base para reportes de laboratorio
- **recursos/** - Imágenes y recursos compartidos (logos, portadas)
- **practica1/** - Laboratorio 1
- **practica2/** - Laboratorio 2
- *... (se irán agregando más prácticas)*

## Uso de la Plantilla

La plantilla LaTeX (`plantillas/machote_arqui.tex`) está lista para usar. Solo necesitas:
1. Copiarla a la carpeta de tu práctica
2. Modificar los datos del reporte en la sección indicada
3. Compilar con pdflatex

## Autor
Moya Monreal Erick Anselmo - 1110604

## Proyecto 2026-1: Space Invaders en C con SDL3

El proyecto `proyecto2026_1` es una aplicacion en C para Visual Studio que usa SDL3. Las librerias necesarias ya estan incluidas en el repositorio:

- `SDL3-devel-3.2.26-VC/`
- `SDL3_ttf-devel-3.2.2-VC/`

### Requisitos

1. Windows.
2. Visual Studio 2022 o superior con la carga de trabajo **Desarrollo para el escritorio con C++**.
3. El componente MSVC compatible con Visual Studio 2022. El archivo `.vcxproj` usa `PlatformToolset` `v143`.

### Como abrirlo y compilarlo en otra computadora

1. Clonar el repositorio:

   ```powershell
   git clone https://github.com/ApoloEM/OyAdeC.git
   cd OyAdeC
   ```

2. Abrir `proyecto2026_1.slnx` con Visual Studio.
3. Seleccionar la configuracion `Debug` y la plataforma `Win32` o `x64`.
4. Compilar con **Build > Build Solution**.
5. Ejecutar con **Debug > Start Without Debugging** o con `Ctrl+F5`.

El proyecto esta configurado con rutas relativas, por lo que no deberia requerir rutas locales como `C:\Users\...`. Al compilar, Visual Studio copia automaticamente `SDL3.dll` y `SDL3_ttf.dll` a la carpeta de salida.

### Archivos importantes

- `Spaceinvaders.c`: codigo fuente principal del juego.
- `proyecto2026_1.vcxproj`: configuracion del proyecto de Visual Studio.
- `proyecto2026_1.slnx`: solucion para abrir el proyecto.

Las carpetas `.vs`, `Debug`, `Release`, `x64` y `x86` no se suben al repositorio porque Visual Studio las genera localmente.
