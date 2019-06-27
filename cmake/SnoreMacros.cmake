set(_SNORE_TOAST_MACRO_DIR ${CMAKE_CURRENT_LIST_DIR})

function(create_icon_rc ICON_PATH RC_NAME)
	get_filename_component(ICON_NAME ${ICON_PATH} NAME)
	file(TO_NATIVE_PATH ${ICON_PATH} ICON_PATH)
	string(REPLACE "\\" "\\\\" ICON_PATH ${ICON_PATH})
	configure_file(${_SNORE_TOAST_MACRO_DIR}/icon.rc.in ${CMAKE_CURRENT_BINARY_DIR}/${ICON_NAME}.rc @ONLY)
	set(${RC_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${ICON_NAME}.rc PARENT_SCOPE)
endfunction()
