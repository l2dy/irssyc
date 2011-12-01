add_library(psyc_core SHARED
  psyc-core.c
  psyc-servers.c
  psyc-channels.c
  psyc-commands.c
  psyc-formats.c
)
target_link_libraries(psyc_core
  -lpsycclient -lpsyccommon -lpsycutil -lpsyc
  ${GLIB2_LIBRARIES}
)
