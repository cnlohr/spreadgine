all : testspread testsurvive testgame set_optical_calibrations testspread2

uname_m := $(shell uname -m)


#USE_GPU:=-DMALI
#LINK_GPU:=-lMali

USE_GPU:=-DRASPI_GPU -DGLES2 -I/opt/vc/include # -DNEED_BUFFER_BITS
LINK_GPU:=-L/opt/vc/lib -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm

#SURVIVE:=`echo ~`/git/libsurvive
SURVIVE:=modules/libsurvive
SURVIVE_CFLAGS:=-I$(SURVIVE)/include -I$(SURVIVE)/redist -DUSE_DOUBLE
SURVIVE_LDFLAGS:=$(SURVIVE)/lib/libsurvive.so -lcblas -llapacke

#	CNHTTP:=cntools/http/http_bsd.o cntools/http/cnhttp.o cntools/http/mfs.o cntools/http/sha1.o
#	RAWDRAW:=rawdraw/CNFG3D.o rawdraw/CNFGEGLDriver.o rawdraw/CNFGFunctions.o
#	RESOURCE_O:=$(CNHTTP) $(RAWDRAW) src/spreadgine.o src/spreadgine_util.o src/spreadgine_remote.o src/objload.o src/spreadgine_vr.o src/spatialloc.o
#	SPREADGINE_C:=

#	CNHTTP:=cntools/http/http_bsd.c cntools/http/cnhttp.c cntools/http/mfs.c cntools/http/sha1.c
#	RAWDRAW:=rawdraw/CNFG3D.c rawdraw/CNFGXDriver.c rawdraw/CNFGFunctions.c src/objload.c
#	RESOURCE_O:=
#	SPREADGINE_C:=src/spreadgine.c src/spreadgine_remote.c src/spreadgine_vr.c src/spreadgine_util.c src/spatialloc.c $(CNHTTP) $(RAWDRAW)


CNHTTP:=cntools/http/http_bsd.o cntools/http/cnhttp.o cntools/http/mfs.o cntools/http/sha1.o
RAWDRAW:=rawdraw/CNFG3D.o rawdraw/CNFGFunctions.o
SPREADGINE_C:=


ifeq ($(uname_m), x86_64)
	RAWDRAW += rawdraw/CNFGXDriver.o
	CFLAGS:= -DCNFGOGL -DHTTP_POLL_TIMEOUT=10 -DCNFG3D_USE_OGL_MAJOR -DHAS_XINERAMA #-DFULL_SCREEN_STEAL_FOCUS
	LDFLAGS:=-lm -lX11 -lXext -lGL -lpthread -lXinerama
else
	RAWDRAW += rawdraw/CNFGEGLDriver.o
	CFLAGS:= -DHTTP_POLL_TIMEOUT=10 -DCNFG3D_USE_OGL_MAJOR $(USE_GPU)
	LDFLAGS:=-lm -lpthread $(LINK_GPU)
endif

CFLAGS+=-Os -g -Iinclude -Icntools/http -Irawdraw -Imodules -Imodules/libsurvive/include -Imodules/libsurvive/redist -Imodules/libsurvive/libs -Imodules/libsurvive/libs/cnmatrix/include
RESOURCE_O:=$(CNHTTP) $(RAWDRAW) src/spreadgine.o src/spreadgine_util.o src/spreadgine_remote.o src/objload.o src/spreadgine_vr.o src/spatialloc.o
RESOURCE_O+=modules/textboxes.o cntools/vlinterm/vlinterm.o
RESOURCE_O+=modules/ttyinput.o

CFLAGS += $(SURVIVE_CFLAGS)
LDFLAGS += $(SURVIVE_LDFLAGS)

testspread : testspread.o $(SPREADGINE_C) $(RESOURCE_O)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

testspread2 : testspread2.o $(SPREADGINE_C) $(RESOURCE_O)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

testsurvive : testsurvive.c $(SPREADGINE_C) $(RESOURCE_O)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) $(SURVIVE_LDFLAGS)

testgame : testgame.c $(SPREADGINE_C) $(RESOURCE_O)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) $(SURVIVE_LDFLAGS)

set_optical_calibrations : set_optical_calibrations.c $(SPREADGINE_C) $(RESOURCE_O)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS) $(SURVIVE_LDFLAGS)

testabnogeo : testabnogeo.c $(SPREADGINE_C) $(RESOURCE_O)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

cntools/http/http_bsd.c :
	git submodule update --init --recursive


clean :
	rm -rf testspread testsurvive testgame testspread2 set_optical_calibrations testabnogeo $(RESOURCE_O)
