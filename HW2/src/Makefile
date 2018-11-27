CC=gcc
C++=g++
CFLAGS=
CPPFLAGS=
CXXFLAGS=

PROG=fm_part
OBJ_DIR=../objs

SOURCES=main.c
SOURCES+=parser.c
SOURCES+=vector.c
SOURCES+=fiduccia.c

OBJECTS := $(addprefix $(OBJ_DIR)/,$(subst .c,.o,$(SOURCES)))
DEPENDS := $(addprefix $(OBJ_DIR)/,$(subst .c,.d,$(SOURCES)))


all: ${PROG}

clean:
	rm -f ${PROG}
	rm -f $(OBJECTS)

${PROG}: $(OBJECTS)
	@echo $^
	$(CC) $^ -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -MMD -MF $(OBJ_DIR)/$*.d -MP -MT'$(OBJ_DIR)/$*.o $(OBJ_DIR)/$*.d' -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -MMD -MF $(OBJ_DIR)/$*.d -MP -MT'$(OBJ_DIR)/$*.o $(OBJ_DIR)/$*.d' -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
