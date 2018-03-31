all : testspread

CNHTTP:=cntools/http/http_bsd.c cntools/http/cnhttp.c cntools/http/mfs.c cntools/http/sha1.c
RAWDRAW:=rawdraw/CNFG3D.c rawdraw/CNFGXDriver.c rawdraw/CNFGFunctions.c
SPREADGINE_C:=src/spreadgine.c src/spreadgine_remote.c $(CNHTTP) $(RAWDRAW)

CFLAGS:=-O1 -g -Iinclude -Icntools/http -Irawdraw -DCNFGOGL -DHTTP_POLL_TIMEOUT=10 -DCNFG3D_USE_OGL_MAJOR
LDFLAGS:=-lm -lX11 -lXext -lGL -lpthread

testspread : testspread.c $(SPREADGINE_C)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean :
	rm -rf testspread
