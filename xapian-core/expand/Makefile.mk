noinst_HEADERS +=\
	expand/esetinternal.h\
	expand/expandweight.h\
	expand/ortermlist.h\
	expand/termlistmerger.h

EXTRA_DIST +=\
	expand/Makefile

lib_src +=\
	expand/bo1eweight.cc\
	expand/esetinternal.cc\
	expand/expandweight.cc\
	expand/ortermlist.cc\
	expand/probeweight.cc
