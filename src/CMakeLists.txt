add_library(SnoreToastActions INTERFACE)
target_include_directories(SnoreToastActions INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/snoretoast>
)
add_library(SnoreToast::SnoreToastActions ALIAS SnoreToastActions)

configure_file(config.h.in config.h @ONLY)
add_library(libsnoretoast STATIC snoretoasts.cpp toasteventhandler.cpp linkhelper.cpp utils.cpp)
target_link_libraries(libsnoretoast PUBLIC runtimeobject shlwapi SnoreToast::SnoreToastActions)
target_compile_definitions(libsnoretoast PRIVATE UNICODE _UNICODE __WRL_CLASSIC_COM_STRICT__ WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_definitions(libsnoretoast PUBLIC __WRL_CLASSIC_COM_STRICT__)
target_include_directories(libsnoretoast PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties(libsnoretoast PROPERTIES EXPORT_NAME LibSnoreToast)
add_library(SnoreToast::LibSnoreToast ALIAS libsnoretoast)
generate_export_header(libsnoretoast)

create_icon_rc(${PROJECT_SOURCE_DIR}/data/zzz.ico TOAST_ICON)
add_executable(snoretoast WIN32 main.cpp ${TOAST_ICON})
target_link_libraries(snoretoast PRIVATE SnoreToast::LibSnoreToast snoreretoastsources)
target_compile_definitions(snoretoast PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
add_executable(SnoreToast::SnoreToast ALIAS snoretoast)

install(TARGETS snoretoast SnoreToastActions EXPORT LibSnoreToastConfig RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
install(FILES snoretoastactions.h ${CMAKE_CURRENT_BINARY_DIR}/config.h DESTINATION include/snoretoast)
install(EXPORT LibSnoreToastConfig DESTINATION lib/cmake/libsnoretoast NAMESPACE SnoreToast::)
