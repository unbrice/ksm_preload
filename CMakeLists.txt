cmake_minimum_required(VERSION 3.4)

project(KSM_PRELOAD)

add_library(ksm_preload SHARED libksm_preload.c)
target_link_libraries(ksm_preload dl)
install(
    TARGETS ksm_preload
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION share/ksm_preload
    NAMELINK_SKIP
)

install(PROGRAMS ksm-wrapper DESTINATION bin)

