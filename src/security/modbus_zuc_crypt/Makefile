FLAGS= -g -shared -o
INCLUDE= -I$(CUBE_PATH)/include/ -I$(CUBESYSPATH)/include -I$(CUBEAPPPATH)/include
CFLAGS= -g -c -fPIC -DUSER_MODE $(INCLUDE)
CC=gcc
CCP=g++
PLUGIN=modbus_zuc_crypt
PLUGIN_OBJ= 
TEST_LIB= -L$(CUBELIBPATH)/lib -lexmodule -lmessage -lmemdb -lconnector -lstruct -lcrypto_func -lpthread 
LOCALLIB=-L$(CUBEAPPPATH)/locallib -lzuc

main: lib$(PLUGIN).so 
lib$(PLUGIN).so:  $(PLUGIN).o $(PLUGIN_OBJ) 
	$(CC) $(FLAGS) lib$(PLUGIN).so $(PLUGIN).o $(PLUGIN_OBJ) $(TEST_LIB) $(LOCALLIB)
	mv lib$(PLUGIN).so $(CUBEAPPPATH)/plugin
	cp $(PLUGIN).cfg $(CUBEAPPPATH)/plugin
$(PLUGIN).o: $(PLUGIN).c $(PLUGIN).h
	$(CC) $(CFLAGS) $(PLUGIN).c
clean:
	rm -f test_main lib$(PLUGIN).so 
	rm  *.o *~
