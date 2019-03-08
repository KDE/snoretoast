
add_library(libsnoretoast STATIC snoretoasts.cpp toasteventhandler.cpp linkhelper.cpp utils.cpp)

add_executable(SnoreToast WIN32 main.cpp ${SNORE_TOAST_DEPS})
target_link_libraries(SnoreToast runtimeobject shlwapi libsnoretoast)

install(TARGETS SnoreToast RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
