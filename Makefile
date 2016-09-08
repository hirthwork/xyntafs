headers=$(shell find src -type f -name \*.hpp)
sources=$(shell find src -type f -name \*.cpp)
CFLAGS = -Ofast -flto
override CFLAGS += -Wall -Wextra -Wpedantic -Werror
override CFLAGS += $(shell pkg-config fuse --cflags --libs)
override CFLAGS += -DFUSE_USE_VERSION=26
override CXXFLAGS += -std=c++1y

xynta: Makefile $(headers) $(sources)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(sources) -o $@
