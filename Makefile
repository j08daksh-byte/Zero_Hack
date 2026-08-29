# NovaCPP - Zero Dependency Web Framework
# Official Single-Command Makefile

CXX ?= g++
CXXFLAGS = -std=c++17 -O2 -I. -Wall
LDFLAGS = -pthread

SOURCES = src/main.cpp src/frontend/App.cpp src/backend/Database.cpp src/backend/Auth.cpp
TEST_SOURCES = tests/test_main.cpp
TARGET = build/NovaCPP
TEST_TARGET = build/nova_tests

.PHONY: all run test clean docker reproducible

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) -o $(TARGET)
	@echo "Build successful: $(TARGET)"

$(TEST_TARGET): $(TEST_SOURCES)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) $(LDFLAGS) -o $(TEST_TARGET)
	@echo "Test suite built: $(TEST_TARGET)"

run: $(TARGET)
	@echo "Starting NovaCPP on http://localhost:8080 ..."
	./$(TARGET)

test: $(TEST_TARGET)
	@echo "Executing zero-dependency test harness..."
	./$(TEST_TARGET)

docker:
	docker build -t novacpp .
	docker run -p 8080:8080 novacpp

clean:
	rm -rf build
