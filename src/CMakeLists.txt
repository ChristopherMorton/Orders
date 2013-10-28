set(LIBS ../lib/SFML_WindowEventManager.cpp)
add_executable(Summoner app.cpp listeners.cpp shutdown.cpp ${LIBS})

if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(${EXECUTABLE_NAME} ${SFML_LIBRARIES})
endif()
