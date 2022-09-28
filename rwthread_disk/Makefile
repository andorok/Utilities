
PHONY = clean
TARGET_NAME = rwthread_disk

#include $(GIPCYDIR)/make/macros.mk
#DIRS := $(GIPCYINC)

DIRS += . include
INC := $(addprefix  -I, $(DIRS))

#VPATH := $(DIR)

SRC := $(wildcard *.cpp)
SOURCE += $(SRC) $(EXTFILES)
#SOURCE += $(SRC)
#OBJ_FILES := $(notdir $(SOURCE:.cpp=.o))
OBJ_FILES := $(SOURCE:.cpp=.o)

all: $(TARGET_NAME)

CFLAGS += -Wall -g -D__linux__ -Wextra -O3
CFLAGS += $(INC)
CXXFLAGS += $(CFLAGS)

LIBS += -L. -Llib_pa -ldl -lc -lrt -lm -lstdc++ -lpthread

LDFLAGS += $(LIBS)

$(TARGET_NAME): $(OBJ_FILES)
	$(LD) -o $(TARGET_NAME) $(notdir $(OBJ_FILES)) $(LDFLAGS)

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)gcc

%.o: %.cpp
	$(CC) $(CFLAGS) -c -MMD $<

include $(wildcard *.d)

clean:
	rm -f *.o *~ core
	rm -f *.d *~ core
	rm -f *.so  *.a
	rm -f *.0
	rm -f $(TARGET_NAME)

distclean: clean
