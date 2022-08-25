target_sources(${MAIN_TARGET_NAME} PRIVATE
    "plugin_editor.cpp"
    "plugin_processor.cpp"
    "settings.cpp"
)

target_include_directories(${MAIN_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)
target_precompile_headers(${MAIN_TARGET_NAME} PUBLIC pch.h)

add_subdirectory(components)
add_subdirectory(node_graph)

    