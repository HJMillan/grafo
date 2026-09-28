# ============================================================
# Copia junto al .exe las DLL que no son de Qt pero que Qt necesita
# ============================================================
# windeployqt solo copia las DLL de Qt. Con el Qt de MSYS2, Qt depende además
# de libstdc++, ICU, zlib, freetype, harfbuzz…, que viven en la misma carpeta
# bin. Sin ellas el .exe solo abre desde un entorno que tenga esa carpeta en el
# PATH («no se encontró libgcc_s_seh-1.dll» al hacer doble clic).
#
# Uso (en modo script, después de windeployqt):
#   cmake -DEXE=<.exe> -DSEARCH_DIR=<bin de Qt> -DOBJDUMP=<objdump> -P CopyRuntimeDeps.cmake
#
# Solo se copian las DLL que se encuentran en SEARCH_DIR: las de Windows se quedan
# fuera. Con el Qt oficial (C:/Qt/…) no hay nada extra que copiar y no hace nada.

foreach (var EXE SEARCH_DIR OBJDUMP)
    if (NOT ${var})
        message(FATAL_ERROR "CopyRuntimeDeps: falta -D${var}=…")
    endif()
endforeach()

get_filename_component(dest "${EXE}" DIRECTORY)

set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${OBJDUMP}")

# Plugins que dejó windeployqt (platforms/qwindows.dll, styles/…, imageformats/…)
file(GLOB plugins "${dest}/*/*.dll")

file(GET_RUNTIME_DEPENDENCIES
        EXECUTABLES "${EXE}"
        LIBRARIES ${plugins}
        # Primero junto al .exe, para que Qt6Core.dll & co. se resuelvan siempre a
        # las copias de windeployqt y no aparezcan dos veces.
        DIRECTORIES "${dest}" "${SEARCH_DIR}"
        PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
        RESOLVED_DEPENDENCIES_VAR resolved
        UNRESOLVED_DEPENDENCIES_VAR unresolved)

file(TO_CMAKE_PATH "${SEARCH_DIR}" search)
string(TOLOWER "${search}/" search)
set(copied 0)
foreach (dll IN LISTS resolved)
    file(TO_CMAKE_PATH "${dll}" dll)
    string(TOLOWER "${dll}" lower)
    string(FIND "${lower}" "${search}" at)
    if (at EQUAL 0)
        file(COPY "${dll}" DESTINATION "${dest}")
        math(EXPR copied "${copied} + 1")
    endif()
endforeach()
message(STATUS "CopyRuntimeDeps: ${copied} DLL de ${SEARCH_DIR} junto a ${EXE}")
