set(APP_FILES app.cpp map.cpp level.cpp units.cpp projectile.cpp orders.cpp focus.cpp savestate.cpp shutdown.cpp util.cpp config.cpp log.cpp)

add_executable(Summoner ${APP_FILES})

target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} ${SFML_LIBRARIES})
