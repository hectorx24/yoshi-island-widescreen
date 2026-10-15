set(YOSHI_RCHEEVOS "${CMAKE_SOURCE_DIR}/upstream/rcheevos")
if(NOT EXISTS "${YOSHI_RCHEEVOS}/include/rc_client.h")
  message(FATAL_ERROR "Pinned rcheevos missing: run python3 tools/fetch_dependencies.py")
endif()
file(GLOB YOSHI_RC_CORE "${YOSHI_RCHEEVOS}/src/rcheevos/*.c"
                        "${YOSHI_RCHEEVOS}/src/rapi/*.c")
add_library(yoshi_rcheevos STATIC ${YOSHI_RC_CORE}
  "${YOSHI_RCHEEVOS}/src/rc_client.c"
  "${YOSHI_RCHEEVOS}/src/rc_client_external.c"
  "${YOSHI_RCHEEVOS}/src/rc_compat.c"
  "${YOSHI_RCHEEVOS}/src/rc_util.c"
  "${YOSHI_RCHEEVOS}/src/rc_version.c"
  "${YOSHI_RCHEEVOS}/src/rhash/md5.c")
set_target_properties(yoshi_rcheevos PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(yoshi_rcheevos PRIVATE RC_DISABLE_LUA)
target_include_directories(yoshi_rcheevos PUBLIC "${YOSHI_RCHEEVOS}/include"
  PRIVATE "${YOSHI_RCHEEVOS}/src")
