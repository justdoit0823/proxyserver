WORK_DIR = $(dir ./)

INCLUDE_DIR = $(WORK_DIR)include/

SOURCE_DIR = $(WORK_DIR)src/

CMACRO = "-D _GNU_SOURCE"

OBJECTS = http.o initnet.o jobqueue.o logging.o server.o \
	pool.o fpoll.o list.o option.o connection.o

target proxyserver: $(OBJECTS)
	gcc -pthread -o proxyserver $(OBJECTS)
	rm -f $(OBJECTS)

http.o: $(SOURCE_DIR)http.c $(INCLUDE_DIR)http.h
	gcc -c $(CMACRO) -I $(INCLUDE_DIR) $(SOURCE_DIR)http.c

initnet.o: $(SOURCE_DIR)initnet.c $(INCLUDE_DIR)initnet.h
	gcc -c -I $(INCLUDE_DIR) $(SOURCE_DIR)initnet.c

jobqueue.o: $(SOURCE_DIR)jobqueue.c $(INCLUDE_DIR)jobqueue.h
	gcc -c -I $(INCLUDE_DIR) $(SOURCE_DIR)jobqueue.c

logging.o: $(SOURCE_DIR)logging.c $(INCLUDE_DIR)logging.h
	gcc -c -I $(INCLUDE_DIR) $(SOURCE_DIR)logging.c

server.o: $(SOURCE_DIR)server.c
	gcc -c -I $(INCLUDE_DIR) $(SOURCE_DIR)server.c

pool.o: $(SOURCE_DIR)pool.c $(INCLUDE_DIR)pool.h
	gcc -c -I $(INCLUDE_DIR) $(SOURCE_DIR)pool.c

fpoll.o: $(SOURCE_DIR)fpoll.c $(INCLUDE_DIR)fpoll.h
	gcc -c -I $(INCLUDE_DIR) $(SOURCE_DIR)fpoll.c

list.o: $(SOURCE_DIR)list.c $(INCLUDE_DIR)list.h
	gcc -c -I $(INCLUDE_DIR) $(SOURCE_DIR)list.c

option.o: $(SOURCE_DIR)option.c $(INCLUDE_DIR)option.h
	gcc -c -I $(INCLUDE_DIR) $(SOURCE_DIR)option.c

connection.o: $(SOURCE_DIR)connection.c
	gcc -c -I $(INCLUDE_DIR) $(SOURCE_DIR)connection.c

install: $(OBJECTS)
	-install $(WORK_DIR)proxyserver /usr/local/bin
	rm -f $(OBJECTS)

clean: 
	rm -f proxyserver $(OBJECTS)

.PHONY: clean
