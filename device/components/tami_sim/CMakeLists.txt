set(TAMI_APP "${CMAKE_CURRENT_LIST_DIR}/../../../app")

idf_component_register(
    SRCS
        "${TAMI_APP}/src/rng.c"
        "${TAMI_APP}/src/names.c"
        "${TAMI_APP}/src/sim.c"
        "${TAMI_APP}/src/anim.c"
        "${TAMI_APP}/src/roster.c"
    INCLUDE_DIRS
        "${TAMI_APP}/include"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format -Wno-error=format-truncation)
