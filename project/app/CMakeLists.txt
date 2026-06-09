add_executable(networking-app
    src/main.cpp
)

target_include_directories(networking-app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(networking-app PRIVATE
    networking-client
    networking-server
)

set(APP_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(APP_RESOURCES_FILES
    GameNetworkingSockets.dll
    libcrypto-3-x64.dll
    libprotobufd.dll
)

foreach(APP_RESOURCE_FILE ${APP_RESOURCES_FILES})
    if(EXISTS "${APP_RESOURCES_DIR}/${APP_RESOURCE_FILE}")
        add_custom_command(TARGET networking-app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${APP_RESOURCES_DIR}/${APP_RESOURCE_FILE}"
                $<TARGET_FILE_DIR:networking-app>
        )
    endif()
endforeach()

# set(APP_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# add_custom_command(TARGET opengl-engine POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#         ${APP_RESOURCES_DIR}/shaders
#         $<TARGET_FILE_DIR:opengl-engine>/shaders
#     COMMENT "Copying application resources"
# )
