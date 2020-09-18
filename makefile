# makefile (PhotoMosaic)
# This is a "child" makefile, which makes one of the static
# libraries (any one) required to build the executable
#
# Include the common definitions
include makefile.inc

# Name of the executable to compile and link
TARGET = PhotoMosaic

# Directories in which to search for source files
DIRS = \
	$(CURDIR)/src \
	$(CURDIR)/src/utilities

# Source files
SRC = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))

# Object files
OBJS = $(addprefix $(OBJDIR),$(SRC:.cpp=.o))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "objs: " $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(BINDIR)$@

$(OBJDIR)%.o: %.cpp
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
