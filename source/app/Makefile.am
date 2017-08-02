bin_PROGRAMS=webpa
webpa_SOURCES=main.c libpd.c
webpa_LDADD = $(top_builddir)/broadband/libbroadband.la
webpa_CFLAGS = -I$(top_srcdir)/include
webpa_CFLAGS += "-DFEATURE_SUPPORT_RDKLOG"
webpa_LDFLAGS = -lm -lwdmp-c -lwrp-c -lnanomsg -lcjson -lmsgpackc -llog4c

