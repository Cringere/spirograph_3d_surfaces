.PHONY: gen_dirs build_library run clean

SRC_FOLDERS = CREngine
#MAIN_FILE = main/MainClass.cpp
MAIN_FILE = main/spiro_3d_v5.cpp

NAME = spiro_surfaces

SRCDIR := src
OBJDIR := obj
DEPDIR := dep

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
CFLAGS = -std=c++11 -Iinclude/ -I/usr/include/freetype2 -g $(DEPFLAGS)
LFLAGS = -lSDL2main -lSDL2 -lSDL2_image -lGLEW -lGLU -lGL -lfreetype -lportaudio -lrt -lm -lasound -pthread -lsndfile

SRC = $(wildcard $(SRCDIR)/*.cpp) $(foreach dir, $(SRCDIR)/$(SRC_FOLDERS), $(wildcard $(dir)/*.cpp))
OBJ = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
DEP := $(patsubst $(SRCDIR)/%.cpp, $(DEPDIR)/%.d, $(SRC))

$(DEPDIR): ; @mkdir -p $@
$(DEP):

OUT = bin/$(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPDIR)/%.d | $(DEPDIR)
	g++ $(CFLAGS) -c $< -o $@ $(LFLAGS)

gen_dirs:
	$(foreach folder,$(SRC_FOLDERS), @mkdir -p $(OBJDIR)/$(folder))
	$(foreach folder,$(SRC_FOLDERS), @mkdir -p $(DEPDIR)/$(folder))

build_library: gen_dirs $(OBJ)
#g++ $(CFLAGS) $(OBJ) -o $(OUT) $(LFLAGS)

build_main: build_library
	g++ $(CFLAGS) -c $(MAIN_FILE) -o $(OBJDIR)/main.o $(LFLAGS)
	g++ $(CFLAGS) $(OBJ) $(OBJDIR)/main.o -o $(OUT) $(LFLAGS)

run: build_library build_main
	clear
	./$(OUT)

clean:
	rm -rf $(OBJDIR)/*
	rm -rf $(DEPDIR)/*
	rm -f $(OUT)

include $(wildcard $(DEP))
