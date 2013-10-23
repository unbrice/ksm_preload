dist_doc_DATA = README

AM_CFLAGS = -Wall -Wextra -Wcast-align -Wpointer-arith -Wcast-align -Wno-sign-compare -Wconversion

lib_LTLIBRARIES = libksm_preload.la
libksm_preload_la_SOURCES = src/libksm_preload.c
libksm_preload_la_LDFLAGS = -avoid-version -module -shared -export-dynamic

EXTRA_DIST = scripts/ksm-wrapper
