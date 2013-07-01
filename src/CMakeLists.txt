
add_executable(SnoreToast main.cpp snoretoasts.cpp toasteventhandler.cpp linkhelper.cpp ${SNORE_TOAST_DEPS})
target_link_libraries(SnoreToast runtimeobject shlwapi)

install(TARGETS SnoreToast RUNTIME DESTINATION bin
                     LIBRARY DESTINATION lib
                     ARCHIVE DESTINATION lib)
