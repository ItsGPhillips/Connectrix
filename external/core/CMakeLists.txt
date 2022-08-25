CPMAddPackage("gh:gabime/spdlog@1.8.2")
subproject_version(spdlog spdlog_version)

add_library(core STATIC)
message(STATUS "Creating core static lib")
target_sources(core PRIVATE
   "logger.cpp"
   "defs.cpp"
)

# find_package(Boost 1.78 REQUIRED)

target_compile_definitions(core PRIVATE SPDLOG_NO_EXCEPTIONS)
target_include_directories(core
   PRIVATE
      ${spdlog_SOURCE_DIR}/include
      ${abseil_SOURCE_DIR}/absl
   INTERFACE
      "core.h"
)

target_link_libraries(core 
   PUBLIC
      spdlog
      absl::flat_hash_map
      range-v3
      absent
)