list(APPEND SOURCES
	module-org-capture.c
)

add_library(module-org-capture MODULE ${SOURCES})

target_compile_definitions(module-org-capture PRIVATE
	G_LOG_DOMAIN=\"module-org-capture\"
)

target_compile_options(module-org-capture PUBLIC
	${EVOLUTION_SHELL_CFLAGS}
	${EVOLUTION_CALENDAR_CFLAGS}
	${EVOLUTION_MAIL_CFLAGS}
	${LIBECAL_CFLAGS}
)

target_include_directories(module-org-capture PUBLIC
	${EVOLUTION_SHELL_INCLUDE_DIRS}
	${EVOLUTION_CALENDAR_INCLUDE_DIRS}
	${EVOLUTION_MAIL_INCLUDE_DIRS}
	${LIBECAL_INCLUDE_DIRS}
)

target_link_libraries(module-org-capture
	${EVOLUTION_SHELL_LDFLAGS}
	${EVOLUTION_CALENDAR_LDFLAGS}
	${EVOLUTION_MAIL_LDFLAGS}
	${LIBECAL_LDFLAGS}
)

install(TARGETS module-org-capture
	DESTINATION ${EVOLUTION_MODULE_DIR}
)
