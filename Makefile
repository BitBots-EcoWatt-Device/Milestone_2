CXX = g++
CXXFLAGS = -std=c++11 -I.
LDFLAGS = -lcurl

SOURCES = ModbusHandler.cpp ProtocolAdapter.cpp Inverter.cpp

all: run

main: main.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) -o main main.cpp $(SOURCES) $(LDFLAGS)

run: main
	./main

clean:
	rm -f main *.o
