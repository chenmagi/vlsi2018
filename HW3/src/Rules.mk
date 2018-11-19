CC=gcc
CXX=g++
CFLAGS?=
CPPFLAGS?=
CXXFLAGS?=

PROG?=
OBJ_DIR?=
SOURCES?=

#OBJECTS := $(addprefix $(OBJ_DIR)/,$(subst .c,.o,$(SOURCES)))
#DEPENDS := $(addprefix $(OBJ_DIR)/,$(subst .c,.d,$(SOURCES)))
CPP_OBJECTS := $(addprefix $(OBJ_DIR)/,$(subst .cpp,.o,$(SOURCES)))
CPP_DEPENDS := $(addprefix $(OBJ_DIR)/,$(subst .cpp,.d,$(SOURCES)))



$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -MMD -MF $(OBJ_DIR)/$*.d -MP -MT'$(OBJ_DIR)/$*.o $(OBJ_DIR)/$*.d' -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -MMD -MF $(OBJ_DIR)/$*.d -MP -MT'$(OBJ_DIR)/$*.o $(OBJ_DIR)/$*.d' -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@
