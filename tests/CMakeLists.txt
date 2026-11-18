add_executable(snoretoast_tests snoretoast_tests.cpp interaction_tests.cpp)
target_link_libraries(snoretoast_tests PRIVATE SnoreToast::LibSnoreToast)
target_include_directories(snoretoast_tests PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src)
target_compile_definitions(snoretoast_tests PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

add_test(NAME snoretoast_tests COMMAND snoretoast_tests)
