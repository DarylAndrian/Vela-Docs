# ════════════════════════════════════════════════════════════════════════════
# DeployQt.cmake — Helper to run windeployqt on Windows after build
# ════════════════════════════════════════════════════════════════════════════
# Usage:
#   include(DeployQt)
#   vela_deploy_qt(target_name)
# ════════════════════════════════════════════════════════════════════════════

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt.exe
        PATHS "${Qt6_DIR}/../../../bin"
              "${Qt6_INSTALL_BINS}"
        NO_DEFAULT_PATH
    )
endif()

function(vela_deploy_qt TARGET)
    if(NOT WIN32 OR NOT WINDEPLOYQT_EXECUTABLE)
        return()
    endif()

    # The executable does not exist when this runs (it is produced by the build),
    # so resolve its path with a generator expression. Testing EXISTS here made
    # every deployment silently no-op.
    add_custom_command(
        TARGET ${TARGET} POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --release
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                "$<TARGET_FILE:${TARGET}>"
        COMMENT "Running windeployqt for ${TARGET}"
        VERBATIM
    )
endfunction()
