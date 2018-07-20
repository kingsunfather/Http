CC = g++ -std=c++11
CC_FLAGS = -W -Wall -Werror -I./
EXEC = my_http

LIB = -pthread

.PHONY: all
all: clean
    $(all)

$(EXEC): server_http.cpp locker.hpp thread_pool.hpp task.hpp
	$(CC) -o $(EXEC) server_http.cpp locker.hpp thread_pool.hpp task.hpp $(LIB)

all: $(EXEC)

clean:
	rm -rf *.o $(EXEC)
