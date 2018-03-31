all : testspread

uname_m := $(shell uname -m)


ifeq ($(uname_m), x86_64)
	CNHTTP:=cntools/http/http_bsd.c cntools/http/cnhttp.c cntools/http/mfs.c cntools/http/sha1.c
	RAWDRAW:=rawdraw/CNFG3D.c rawdraw/CNFGXDriver.c rawdraw/CNFGFunctions.c
	RESOUCE_O:=
	SPREADGINE_C:=src/spreadgine.c src/spreadgine_remote.c $(CNHTTP) $(RAWDRAW)

	CFLAGS:=-O1 -g -Iinclude -Icntools/http -Irawdraw -DCNFGOGL -DHTTP_POLL_TIMEOUT=10 -DCNFG3D_USE_OGL_MAJOR
	LDFLAGS:=-lm -lX11 -lXext -lGL -lpthread
else
	CNHTTP:=cntools/http/http_bsd.o cntools/http/cnhttp.o cntools/http/mfs.o cntools/http/sha1.o
	RAWDRAW:=rawdraw/CNFG3D.o rawdraw/CNFGEGLDriver.o rawdraw/CNFGFunctions.o
	RESOUCE_O:=$(CNHTTP) $(RAWDRAW) src/spreadgine.o src/spreadgine_remote.o
	SPREADGINE_C:=
	CFLAGS:=-O1 -g -Iinclude -Icntools/http -Irawdraw -DHTTP_POLL_TIMEOUT=10 -DCNFG3D_USE_OGL_MAJOR -DMALI
	LDFLAGS:=-lm -lMali -lpthread
endif

testspread : testspread.o $(SPREADGINE_C) $(RESOUCE_O)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf testspread $(RESOUCE_O)
