set(APP_FILES app.cpp level.cpp listeners.cpp shutdown.cpp log.cpp)

add_executable(Summoner ${APP_FILES})

target_link_libraries(${EXECUTABLE_NAME} ${LIBRARY_NAME} ${SFML_LIBRARIES})
