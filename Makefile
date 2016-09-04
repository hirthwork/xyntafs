headers=$(shell find src -type f -name \*.hpp)
sources=$(shell find src -type f -name \*.cpp)
CFLAGS = -Ofast -flto
CFLAGS += -Wall -Wextra -Wpedantic -Werror
CFLAGS += $(shell pkg-config fuse --cflags --libs) -DFUSE_USE_VERSION=29
CXXFLAGS += -std=c++1y

xynta: Makefile $(headers) $(sources)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(sources) -o $@
