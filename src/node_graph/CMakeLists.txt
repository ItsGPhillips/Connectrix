target_sources(${MAIN_TARGET_NAME} PRIVATE
   "node_graph.cpp"
   "internal_plugins/plugin_format.cpp"
   "internal_plugins/plugin_instance.cpp"
)

add_subdirectory(editor)