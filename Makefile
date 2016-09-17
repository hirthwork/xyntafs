CFLAGS = -Ofast -flto
override CFLAGS += -Wall -Wextra -Wpedantic -Werror
override CFLAGS += $(shell pkg-config fuse --cflags --libs)
override CFLAGS += -DFUSE_USE_VERSION=26 -Isrc

gcc-version = \
    $(firstword \
	$(subst ., ,$(shell $(CXX) --version | awk '/^g\+\+/{print $$NF;}')))
obsolete-compiler = \
    $(shell [ -n "$(gcc-version)" ] && [ "$(gcc-version)" -lt 6 ] && echo true)

ifeq ($(obsolete-compiler),true)
    override CXXFLAGS += -std=c++1y -Dshared_mutex=shared_timed_mutex
else
    override CXXFLAGS += -std=c++1z
endif

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
