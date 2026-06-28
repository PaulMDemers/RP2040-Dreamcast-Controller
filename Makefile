CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude
HOST_BUILD_DIR := build-host
HOST_TEST_BIN := $(HOST_BUILD_DIR)/dreamcast_core_tests
HOST_SIM_BIN := $(HOST_BUILD_DIR)/maple_sim

CORE_SOURCES := \
	src/maple_packet.cpp \
	src/maple_wire.cpp \
	src/maple_pio_buffer.cpp \
	src/maple_session.cpp \
	src/dreamcast_controller.cpp \
	src/controller_input_parser.cpp \
	src/controller_device.cpp

TEST_SOURCES := \
	tests/test_main.cpp

.PHONY: test sim clean

test: $(HOST_TEST_BIN)
	$(HOST_TEST_BIN)

sim: $(HOST_SIM_BIN)
	$(HOST_SIM_BIN)

$(HOST_TEST_BIN): $(CORE_SOURCES) $(TEST_SOURCES) | $(HOST_BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CORE_SOURCES) $(TEST_SOURCES) -o $@

$(HOST_SIM_BIN): $(CORE_SOURCES) tools/maple_sim.cpp | $(HOST_BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CORE_SOURCES) tools/maple_sim.cpp -o $@

$(HOST_BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(HOST_BUILD_DIR)
