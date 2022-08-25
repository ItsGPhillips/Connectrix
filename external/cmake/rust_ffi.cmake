
function(_verify_args)
   if ($<BOOL:${lib_PACKAGE}>)
      message(FATAL_ERROR "No Package specified")
   endif()
   if ($<BOOL:${lib_CXX_FILE}>)
      message(FATAL_ERROR "No CXX File Set")
   endif()
   if ($<BOOL:${lib_DIRECTORY}>)
      message(FATAL_ERROR "No CXX File Set")
   endif()
endfunction()

# PACKAGE - the name of the library
# CXX_FILE - the name of the cxx file
# DIRECTORY - the root dir of the rust project
function(add_rust_library)
   set(options)
   set(args PACKAGE CXX_FILE DIRECTORY)
   set(list_args)
   cmake_parse_arguments(
      PARSE_ARGV 0
      lib
      "${options}"
      "${args}"
      "${list_args}"
   )

   _verify_args()
            
   if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(CARGO_CMD cargo build)
      set(TARGET_DIR "debug")
   else()
      set(CARGO_CMD cargo build --release)
      set(TARGET_DIR "release")
   endif()

   if (WIN32)
      set(libext "lib")
   elseif(UNIX)
      set(libext "a")
   endif()

   set(target_name "rust_${lib_PACKAGE}" PARENT_SCOPE)
   set(target_name "rust_${lib_PACKAGE}")
   
   set(intermediate_dir ${CMAKE_CURRENT_BINARY_DIR}/${lib_PACKAGE})

   set(cxxbridge_source_out_dir  ${intermediate_dir}/out/src)
   set(cxxbridge_source_out      ${cxxbridge_source_out_dir}/${lib_PACKAGE}.cpp)
   
   set(cxx_header_include_dir    ${intermediate_dir}/out/include)
   set(cxxbridge_header_out_dir  ${cxx_header_include_dir}/${lib_PACKAGE})
   set(cxxbridge_header_out      ${cxxbridge_header_out_dir}/${lib_PACKAGE}.h)
   
   set(rust_static_lib_out_dir   ${intermediate_dir}/out/lib-$<$<CONFIG:DEBUG>:debug>$<$<CONFIG:NDEBUG>:release>)
   set(rust_static_lib_out ${rust_static_lib_out_dir}/${lib_PACKAGE}.${libext})

   set(rust_target_dir ${intermediate_dir}/target)
   
   add_library(${target_name} STATIC)

   add_custom_command(
      OUTPUT ${rust_static_lib_out} ${cxxbridge_source_out} ${cxxbridge_header_out}
      COMMAND ${CARGO_CMD} --target-dir ${rust_target_dir} --out-dir ${rust_static_lib_out_dir} -Z unstable-options
      COMMAND cxxbridge ${lib_CXX_FILE} --header --output ${cxxbridge_header_out}
      COMMAND cxxbridge ${lib_CXX_FILE} --output ${cxxbridge_source_out}
      COMMENT "Building ${lib_PACKAGE}"
      WORKING_DIRECTORY ${lib_DIRECTORY}
      VERBATIM
   )

   target_compile_definitions(${target_name} PUBLIC _ITERATOR_DEBUG_LEVEL)

   target_sources(${target_name} PRIVATE ${cxxbridge_source_out})

   target_link_directories(${target_name} PRIVATE ${rust_static_lib_out_dir})
   target_link_libraries(${target_name} 
      PUBLIC
         ${rust_static_lib_out}
         recommended_config_flags
         recommended_lto_flags
         recommended_warning_flags
   )

   target_include_directories(${target_name} PUBLIC ${cxx_header_include_dir})

endfunction()

# cxxbridge src/lib.rs --output D:/Dev/Connectrix_v2/build/rust/out/src/rust.cpp --header D:/Dev/Connectrix_v2/build/rust/out/include/rust/rust.h