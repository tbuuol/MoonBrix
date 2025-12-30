# Compiler
CXX = g++

# Flags
CXXFLAGS = -Wall -pthread

# Ziel
TARGET = MoonBrix

# Quellen
SRC := $(wildcard bin/*.cpp)

# Linux-Build (default)
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lcurl -lssl -lcrypto

# Aufräumen
clean:
	rm -f $(TARGET) *.o
