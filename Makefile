APP_NAME := HFTtool

CPP := g++

CPPFLAGS += -c -Wall -std=c++17 -fno-strict-overflow

PATH_INCLUDES := .
PATH_INCLUDES += ./gui_imgui/imgui
PATH_INCLUDES += ./gui_imgui/implot
PATH_INCLUDES += /usr/local/include
PATH_INCLUDES += /usr/include/x86_64-linux-gnu
PATH_INCLUDES += /usr/include/x86_64-linux-gnu/c++/7

LDFLAGS := 

SHARED_LIBS	:= -lz -lpthread -lssl -lcrypto
SHARED_LIBS	+= -lboost_thread -lboost_system -lboost_iostreams
SHARED_LIBS	+= -lboost_coroutine -lboost_chrono -lboost_date_time
SHARED_LIBS	+= -lglfw -lGL -lGLU -lXrandr -lXxf86vm -lX11 -lrt -ldl

STATIC_LIBS := 

# ------------------------------------------------------------------------------
# Paths
# ------------------------------------------------------------------------------

PATH_OBJECTS := __obj
SOURCES_FULL_PATHS = $(shell find . -name '*.cpp')
SOURCES_EXTENSION_O := $(patsubst %.cpp,%.o,$(SOURCES_FULL_PATHS))
OBJECTS_FULL_PATHS := $(addprefix $(PATH_OBJECTS)/, $(SOURCES_EXTENSION_O))

# ------------------------------------------------------------------------------
# Rules
# ------------------------------------------------------------------------------

__obj/./%.o: %.cpp
	$(CPP) $< $(CPPFLAGS) \
	$(addprefix -I, $(PATH_INCLUDES)) $(addprefix -D,$(DEFINES)) -o $@

# ------------------------------------------------------------------------------
# .PHONY
# ------------------------------------------------------------------------------

.PHONY: objects_dirs all debug

# ------------------------------------------------------------------------------
# all target
# ------------------------------------------------------------------------------

all: objects_dirs $(APP_NAME)
	strip $(APP_NAME)

# ------------------------------------------------------------------------------
# debug target
# ------------------------------------------------------------------------------

debug: objects_dirs $(APP_NAME)

# ------------------------------------------------------------------------------
# objects_dirs
# ------------------------------------------------------------------------------

objects_dirs:
	@$(foreach dir,$(SOURCES_FULL_PATHS),mkdir -p ${PATH_OBJECTS}/$(dir $(dir));)

# ------------------------------------------------------------------------------
# whole app
# ------------------------------------------------------------------------------

$(APP_NAME): $(OBJECTS_FULL_PATHS)
	$(CPP) -o $@ $^ $(LDFLAGS) $(SHARED_LIBS) $(STATIC_LIBS)

# ------------------------------------------------------------------------------
# clean
# ------------------------------------------------------------------------------

clean:
	@$ if [ -e ${APP_NAME} ]; then rm ${APP_NAME}; fi;
	@$ rm -rf $(PATH_OBJECTS)/*
