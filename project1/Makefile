#sample makefile for CPSC 4040/6040
#author: Ioannis Karamouzas (ioannis@clemson.edu)

#specify your compiler
CC      = g++

# auxiliary flags
CFLAGS	= -g

#first set up the platform dependent variables
ifeq ("$(shell uname)", "Darwin")
  LDFLAGS     = -framework Foundation -framework GLUT -framework OpenGL -lOpenImageIO -lm
else
  ifeq ("$(shell uname)", "Linux")
    LDFLAGS     = -L /usr/lib64/ -lglut -lGL -lGLU -lOpenImageIO -lm
  endif
endif

#this will be the name of your executable
PROJECT = imgview

#list a .o file for each .cpp file that you will compile
#this makefile will compile each cpp separately before linking
OBJECTS = imgview.o

#this does the linking step  
all: ${PROJECT}
${PROJECT} : ${OBJECTS} 
	${CC} ${CFLAGS} -o ${PROJECT} ${OBJECTS} ${LDFLAGS} 

#this generically compiles each .cpp to a .o file
%.o: %.cpp
	${CC} -c ${CFLAGS} $<

#it does not check for .h files dependencies, but you could add that, e.g. 
#somfile.o    : somefile.cpp someheader.h
#	${CC} ${CFLAGS} -c somefile.cpp

	
#this will clean up all temporary files created by make all
clean:
	rm -f core.* *.o *~ ${PROJECT}
