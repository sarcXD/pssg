mkdir temp

CL /Fo"./temp/" /EHsc /c /Zi lib\str\p_strlib.c lib\file\p_flib.c lib\structs\p_structs.c
CL /Fo"./temp/" /c /Zi main.c 
LINK /LIBPATH:"./temp" /DEBUG p_strlib.obj p_flib.obj p_structs.obj main.obj /OUT:pssg.exe
@rem rmdir /s /q temp