lib_LTLIBRARIES=libbroadband.la
libbroadband_la_SOURCES=ssp_messagebus_interface.c ssp_main.c ssp_action.c cosa_webpa_dml.c cosa_webpa_internal.c cosa_webpa_apis.c plugin_main.c webpa_adapter.c webpa_internal.c webpa_table.c webpa_parameter.c webpa_attribute.c webpa_notification.c
libbroadband_la_CFLAGS = -I$(top_srcdir)/broadband/include $(DBUS_CFLAGS)
libbroadband_la_CFLAGS += -I$(top_srcdir)/include
libbroadband_la_LDFLAGS = -lccsp_common -lm -lpthread -lwdmp-c -lcjson -llog4c
GIT_VERSION := $(shell git log -1 --format=%h)
libbroadband_la_CFLAGS += "-DWEBPA_GIT_VERSION=\"$(GIT_VERSION)\""
