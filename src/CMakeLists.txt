set(APP_FILES app.cpp map.cpp level.cpp units.cpp effects.cpp orders.cpp focus.cpp savestate.cpp animation.cpp shutdown.cpp util.cpp config.cpp log.cpp pugixml.cpp)

add_executable(Summoner ${APP_FILES})

target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} ${SFML_LIBRARIES})
