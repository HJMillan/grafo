# ============================================================
# 🧩 CMakeLists.txt – ShortestPathApp (Qt 6)
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
# 🔍 Buscar Qt6
# ------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)

# ------------------------------------------------------------
# ⚠️ Advertencias del compilador
# ------------------------------------------------------------
function(enable_warnings target)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    elseif (MSVC)
        target_compile_options(${target} PRIVATE /W4)
    endif()
endfunction()

# ------------------------------------------------------------
# 🧠 Núcleo: modelo del grafo y algoritmos (solo QtCore)
# ------------------------------------------------------------
add_library(graphcore STATIC
        src/Graph.cpp src/Graph.h
        src/ShortestPath.cpp src/ShortestPath.h
)
target_include_directories(graphcore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(graphcore PUBLIC Qt6::Core)
enable_warnings(graphcore)

# ------------------------------------------------------------
# 🖥️ Aplicación
# ------------------------------------------------------------
file(GLOB APP_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui"
)
list(FILTER APP_SOURCES EXCLUDE REGEX "/src/(Graph|ShortestPath)\\.(cpp|h)$")

add_executable(${PROJECT_NAME} ${APP_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ui")
target_link_libraries(${PROJECT_NAME} PRIVATE graphcore Qt6::Widgets)
enable_warnings(${PROJECT_NAME})

if (WIN32)
    # Busca windeployqt junto a qmake de la instalación de Qt encontrada
    get_target_property(QT_QMAKE_LOCATION Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_LOCATION}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6 HINTS "${QT_BIN_DIR}")
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" --compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
                COMMENT "windeployqt: copiando DLLs de Qt y runtime de MinGW junto al .exe")
    else()
        message(WARNING "No se encontró windeployqt en ${QT_BIN_DIR}")
    endif()
endif()

# ------------------------------------------------------------
# 🧪 Tests (ctest)
# ------------------------------------------------------------
enable_testing()

add_executable(tst_shortestpath tests/tst_shortestpath.cpp)
target_link_libraries(tst_shortestpath PRIVATE graphcore Qt6::Test)
enable_warnings(tst_shortestpath)
add_test(NAME tst_shortestpath COMMAND tst_shortestpath)
