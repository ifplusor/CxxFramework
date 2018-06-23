find_package(PkgConfig REQUIRED)

OPTION(USE_JSONCPP "use jsoncpp library" TRUE)
if (USE_JSONCPP)
    message("config jsoncpp library")
    pkg_check_modules(JSONCPP jsoncpp)
    link_libraries(${JSONCPP_LIBRARIES})
    message(${JSONCPP_LIBRARIES})
endif (USE_JSONCPP)

