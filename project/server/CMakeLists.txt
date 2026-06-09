add_library(networking-server STATIC
    src/server.cpp
)

target_include_directories(networking-server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(networking-server PUBLIC
    steam
    buffer
)

target_link_libraries(networking-server PRIVATE
    fmt
)

# set(APP_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# add_custom_command(TARGET opengl-engine POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#         ${APP_RESOURCES_DIR}/shaders
#         $<TARGET_FILE_DIR:opengl-engine>/shaders
#     COMMENT "Copying application resources"
# )
