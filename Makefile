CXX = g++

CXXFLAGS = -O2 -masm=intel -Wall
LDFLAGS = -shared -static -static-libgcc -static-libstdc++ -s

IdleCPUFix.dll: main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
