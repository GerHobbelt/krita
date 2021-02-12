set(kritaantialias_SOURCES antialias.cpp )
add_library(kritaantialias MODULE ${kritaantialias_SOURCES})
target_link_libraries(kritaantialias kritaui)
install(TARGETS kritaantialias  DESTINATION ${KRITA_PLUGIN_INSTALL_DIR})
