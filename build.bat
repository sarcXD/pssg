mkdir temp
CL /Fo"./temp/" /EHsc /c lib\str\p_strlib.c lib\file\p_flib.c 
CL /Fo"./temp/" /c main.c 
LINK /LIBPATH:"./temp" p_strlib.obj p_flib.obj main.obj /OUT:pssg.exe