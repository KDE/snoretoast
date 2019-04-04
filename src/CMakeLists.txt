
add_library(libsnoretoast SHARED snoretoasts.cpp toasteventhandler.cpp linkhelper.cpp utils.cpp)
target_link_libraries(libsnoretoast PUBLIC runtimeobject shlwapi)
target_compile_definitions(libsnoretoast PRIVATE UNICODE _UNICODE __WRL_CLASSIC_COM_STRICT__ WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_definitions(libsnoretoast PUBLIC __WRL_CLASSIC_COM_STRICT__)
target_include_directories(libsnoretoast PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

generate_export_header(libsnoretoast)

add_executable(SnoreToast WIN32 main.cpp ${SNORE_TOAST_DEPS})
target_link_libraries(SnoreToast libsnoretoast)
target_compile_definitions(SnoreToast PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_definitions(SnoreToast PRIVATE SNORETOAST_CALLBACK_UUID="{383803B6-AFDA-4220-BFC3-0DBF810106BF}")

install(TARGETS libsnoretoast SnoreToast RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES snoretoastactions.h DESTINATION includes/snoretoast)
