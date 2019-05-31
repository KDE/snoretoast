
add_library(libsnoretoast STATIC snoretoasts.cpp toasteventhandler.cpp linkhelper.cpp utils.cpp)
target_link_libraries(libsnoretoast PUBLIC runtimeobject shlwapi)
target_compile_definitions(libsnoretoast PRIVATE UNICODE _UNICODE __WRL_CLASSIC_COM_STRICT__ WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_definitions(libsnoretoast PRIVATE
    SNORETOAST_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    SNORETOAST_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    SNORETOAST_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
target_compile_definitions(libsnoretoast PUBLIC __WRL_CLASSIC_COM_STRICT__)
target_include_directories(libsnoretoast PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties(libsnoretoast PROPERTIES EXPORT_NAME LibSnoreToast)
add_library(SnoreToast::LibSnoreToast ALIAS libsnoretoast)

add_library(libsnoretoast_actions INTERFACE)
target_include_directories(libsnoretoast_actions INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/includes/snoretoast>
)
add_library(SnoreToast::LibSnoreToastActions ALIAS libsnoretoast_actions)

generate_export_header(libsnoretoast)

add_executable(SnoreToast WIN32 main.cpp ${SNORE_TOAST_DEPS})
target_link_libraries(SnoreToast libsnoretoast)
target_compile_definitions(SnoreToast PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
add_executable(SnoreToast::SnoreToast ALIAS SnoreToast)


# if there are changes to the callback mechanism we need to change the uuid for the activator SNORETOAST_CALLBACK_UUID
target_compile_definitions(SnoreToast PRIVATE
    SNORETOAST_CALLBACK_UUID="{383803B6-AFDA-4220-BFC3-0DBF810106BF}"
)

install(TARGETS libsnoretoast SnoreToast libsnoretoast_actions EXPORT LibSnoreToastConfig RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES snoretoastactions.h DESTINATION includes/snoretoast)
install(EXPORT LibSnoreToastConfig DESTINATION lib/cmake/libsnoretoast NAMESPACE SnoreToast::)
