add_library(psyc_core SHARED
  psyc-core.c
  psyc-servers.c
  psyc-channels.c
  psyc-commands.c
  psyc-formats.c
)

target_link_libraries(psyc_core
  -lpsyc -lpsycclient -lpsyccommon -lpsycutil
  ${GLIB2_LIBRARIES}
)

install(TARGETS psyc_core DESTINATION ${IRSSI_MODULES_DIR})
