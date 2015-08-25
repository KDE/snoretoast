
add_library(libsnoretoast STATIC snoretoasts.cpp toasteventhandler.cpp linkhelper.cpp)

add_executable(SnoreToast main.cpp ${SNORE_TOAST_DEPS})
target_link_libraries(SnoreToast runtimeobject shlwapi libsnoretoast)

add_executable(SnoreToastGui WIN32 main.cpp ${SNORE_TOAST_DEPS})
target_link_libraries(SnoreToastGui runtimeobject shlwapi libsnoretoast)
set_property( TARGET SnoreToastGui
              APPEND
              PROPERTY COMPILE_DEFINITIONS BUILD_GUI)

install(TARGETS SnoreToast SnoreToastGui    RUNTIME DESTINATION bin
                                            LIBRARY DESTINATION lib
                                            ARCHIVE DESTINATION lib)
