all : testspread

CNHTTP:=cntools/http/http_bsd.c cntools/http/cnhttp.c cntools/http/mfs.c cntools/http/sha1.c
RAWDRAW:=rawdraw/CNFG3D.c rawdraw/CNFGXDriver.c 

SPREADGINE_C:=src/spreadgine.c $(CNHTTP) $(RAWDRAW)


testspread : testspread.c $(SPREADGINE_C)
