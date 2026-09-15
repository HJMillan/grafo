# ============================================================
# 🧩 CMakeLists.txt – Plantilla base estable para proyectos Qt
# ============================================================

cmake_minimum_required(VERSION 3.22)
project(ShortestPathApp LANGUAGES CXX)

# ------------------------------------------------------------
# ⚙️ Configuración general del compilador
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ------------------------------------------------------------
# 📁 Rutas adicionales para los archivos .ui
# ------------------------------------------------------------
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")

# ------------------------------------------------------------
# 🔍 Buscar Qt6 (Widgets)
# ------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# ------------------------------------------------------------
# 🧠 Descubrir automáticamente todos los archivos del proyecto
# ------------------------------------------------------------
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui"
)

# ------------------------------------------------------------
# ⚙️ Crear el ejecutable con todos los archivos
# ------------------------------------------------------------
add_executable(${PROJECT_NAME} ${APP_SOURCES})

# ------------------------------------------------------------
# 📦 Incluir rutas de headers y UI
# ------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)

# ------------------------------------------------------------
# 🔗 Vincular librerías de Qt
# ------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

if (WIN32)
    # Encuentra windeployqt en el bin de Qt (usamos CMAKE_PREFIX_PATH del preset)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${CMAKE_PREFIX_PATH}/bin")
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" --compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
                COMMENT "windeployqt: copiando DLLs de Qt y runtime de MinGW junto al .exe")
    else()
        message(WARNING "No se encontró windeployqt en ${CMAKE_PREFIX_PATH}/bin")
    endif()
endif()


# ------------------------------------------------------------
# 🧩 Notas finales
# ------------------------------------------------------------
# ✔️ Detecta automáticamente nuevos .cpp/.h/.ui.
# ✔️ Compatible con presets (mingw-debug / mingw-release).
# ✔️ No requiere modificar rutas ni toolchains.
# ✔️ Evita errores de AutoUic (ya incluye /ui en el path).
