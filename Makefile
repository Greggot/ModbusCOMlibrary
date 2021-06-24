CC = gcc
SOURCE = DPS.c #Remove for library only
SOURCE_DLL = Modbuslib.c

DLL_OBJ = $(patsubst %.c, %.o, $(SOURCE_DLL))

PROGRAM = DPS #Remove for library only
DLL = Modbus

all: $(DLL)#Remove for library only
	$(CC) -o $(PROGRAM) $(SOURCE) -L./ -l$^ 

$(DLL): $(DLL_OBJ)
	$(CC) -shared -o $@.dll $^
	
$(DLL_OBJ) : $(SOURCE_DLL)
	$(CC) -c -DBUILD_DLL $^

clear: 
	rm -f $(PROGRAM).exe $(DLL).dll $(DLL_OBJ)