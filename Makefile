CFLAGS = -Ofast -pipe -flto
override CFLAGS += -Wall -Wextra -Wpedantic -Werror -Wstrict-overflow=5
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
testdir = $(builddir)/test
headers = $(shell find src -type f -name \*.hpp)
sources = $(shell find src -type f -name \*.cpp)
tests = basic-listing
tests-base = $(addprefix $(testdir)/,$(tests))
tests-done = $(addsuffix /done,$(tests-base))
target = $(builddir)/xynta

.PHONY: clean test

$(target): Makefile $(headers) $(sources) | $(builddir)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(sources) -o $@

$(builddir):
	mkdir -p $(builddir)

$(testdir):
	mkdir -p $(testdir)

clean:
	for mountpoint in $(tests-base); do \
	    fusermount -q -u $$mountpoint/mount || true; \
	done
	rm -rf $(builddir)

test: $(tests-done)

$(tests-done) : $(testdir)/%/done : $(testdir)/%/run
	fusermount -q -u $(dir $@)mount || true
	touch $@ $(dir $@){run,prepare}

$(addsuffix /run,$(tests-base)) : $(testdir)/%/run : $(testdir)/%/prepare

$(addsuffix /prepare,$(tests-base)) : %/prepare : $(target) | $(testdir)
	fusermount -q -u $(dir $@)mount || true
	rm -rf $(dir $@){data,mount}
	mkdir -p $(dir $@){data,mount}

$(testdir)/basic-listing/run:
	mkdir $(dir $@)data/dir1
	mkdir $(dir $@)data/dir2
	mkdir $(dir $@)data/dir2/subdir1
	echo "file1 content" > $(dir $@)data/dir1/file1
	echo "file2 content" > $(dir $@)data/dir1/file2
	echo "file3 content" > $(dir $@)data/dir2/subdir1/file3
	echo "file4 content" > $(dir $@)data/dir2/subdir1/file4
	setfattr -n user.xynta.tags -v 'tag1 tag2' $(dir $@)data/dir1/file2
	setfattr -n user.xynta.tags -v 'tag1' $(dir $@)data/dir2/subdir1/file3
	setfattr -n user.xynta.tags -v 'tag1 tag3' \
	    $(dir $@)data/dir2/subdir1/file4
	$(target) -d $(abspath $(dir $@)data) -m0 $(dir $@)mount
	test "$$(echo -e 'file1\nfile2\ntag1\ntag2')" = \
	    "$$(ls $(dir $@)mount/dir1)"
