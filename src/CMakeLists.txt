find_package(CURL REQUIRED)

add_executable(${PROJECT_NAME} main.cpp ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)

if(INSTALL_BIN_DIR MATCHES OFF) 
    set(INSTALL_DIR "/usr/local/bin")
else()
    set(INSTALL_DIR ${INSTALL_BIN_DIR})
endif()

if(USE_LOCALHOST_API MATCHES ON)
    ADD_DEFINITIONS(-DLOCAL_API_URL=true)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_DIR})
