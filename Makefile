CC = g++
EXE = mesh2
IMGUI_DIR = ./dependencies/imgui
SOURCES = main.cpp mesh.cpp obj.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

CXXFLAGS = -std=c++11 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -g -Wall -Wformat $(pkg-config --cflags glfw3)
LIBS = -lglfw -lGLEW -lGL -lm

ECHO_MESSAGE = "linux compiled $(EXE)"

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo $(ECHO_MESSAGE)

obj.o: obj.cpp obj.hpp mesh.hpp
	$(CXX) -c $<

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
