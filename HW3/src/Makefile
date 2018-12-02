PROG=hw3
OBJ_DIR=./.build

SOURCES=parser.cpp
SOURCES+=datatypes.cpp
SOURCES+=algo.cpp
SOURCES+=main.cpp
SOURCES+=sa.cpp
SOURCES+=view.cpp

#CPPFLAGS+=-std=c++11 #-DBOOST_DISABLE_ASSERTS
#CPPFLAGS+=-DMYDEBUG
#CPPFLAGS+=-DBOOST_DISABLE_ASSERTS
#CPPFLAGS+=-DUSE_UI
LDFLAGS=-lboost_filesystem  -lopencv_core -lopencv_imgproc -lopencv_highgui


include Rules.mk


all: ${PROG}
	cp -f ${PROG} ../bin
	cp -f ${PROG} seed_gen

clean:
	rm -f ${PROG}
	rm -f $(OBJECTS)
	rm -f $(CPP_OBJECTS)
	rm -f $(DEPENDS)
	rm -f $(CPP_DEPENDS)
	rm -f seed_gen

${PROG}: $(CPP_OBJECTS)
	$(CXX) ${LDFLAGS} $^ -o $@

