CFLAGS = -Ofast -flto
override CFLAGS += -Wall -Wextra -Wpedantic -Werror
override CFLAGS += $(shell pkg-config fuse --cflags --libs)
override CFLAGS += -DFUSE_USE_VERSION=26
override CXXFLAGS += -std=c++14

builddir = build
headers = $(shell find src -type f -name \*.hpp)
sources = $(shell find src -type f -name \*.cpp)

.PHONY: clean

$(builddir)/xynta: Makefile $(headers) $(sources) | $(builddir)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(sources) -o $@

$(builddir):
	mkdir $(builddir)

clean:
	rm -rf $(builddir)
