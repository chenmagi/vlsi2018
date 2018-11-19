PROG=hw3
OBJ_DIR=./.build

SOURCES=parser.cpp
SOURCES+=datatypes.cpp
SOURCES+=algo.cpp
SOURCES+=main.cpp
CPPFLAGS+=-std=c++11 #-DBOOST_DISABLE_ASSERTS
LDFLAGS=-lboost_system


include Rules.mk


all: ${PROG}

clean:
	rm -f ${PROG}
	rm -f $(OBJECTS)
	rm -f $(CPP_OBJECTS)
	rm -f $(DEPENDS)
	rm -f $(CPP_DEPENDS)

${PROG}: $(CPP_OBJECTS)
	$(CXX) ${LDFLAGS} $^ -o $@

