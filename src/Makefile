#OBJS specifies which files to compile as part of the project
OBJS =  Program_All_in_one_file.cpp

#CC specifies which compiler we're using
CC = g++

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings, removed non-optimization -O0
COMPILER_FLAGS = -Wall -std=c++11 -g

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lSDL2 -lfftw3 -lm -lpthread

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = program

#This is the target that compiles our executable
all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
clean:
	rm -f program
