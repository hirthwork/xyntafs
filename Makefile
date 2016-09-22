CFLAGS = -O2 -pipe -flto
override CFLAGS += -Wall -Wextra -Wpedantic -Werror -Wstrict-overflow=5
override CFLAGS += $(shell pkg-config fuse --cflags)
override CFLAGS += -DFUSE_USE_VERSION=26 -Isrc

override LDFLAGS += $(shell pkg-config fuse --libs)

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

COMPILERS_LIST = g++-4.9.3 g++-5.4.0 g++-6.2.0 clang++

builddir = build
testdir = $(builddir)/test
headers = $(shell find src -type f -name \*.hpp)
sources = $(shell find src -type f -name \*.cpp)
tests = basic-listing
tests-base = $(addprefix $(testdir)/,$(tests))
tests-done = $(addsuffix /done,$(tests-base))
target = $(builddir)/xynta

.PHONY: clean test full-test

$(target): Makefile $(headers) $(sources) | $(builddir)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $(sources) -o $@

$(builddir):
	mkdir -p $(builddir)

$(testdir):
	mkdir -p $(testdir)

clean:
	for mountpoint in $(tests-base); do \
	    fusermount -q -u $$mountpoint/mount || true; \
	done
	rm -rf $(builddir)

full-test:
	for x in $(COMPILERS_LIST); do \
	    echo -e "\n[Running test for $$x]"; \
	    $(MAKE) CXX=$$x builddir=$(builddir)/$$x test || exit 1; \
	done

test: $(tests-done)

$(tests-done) : $(testdir)/%/done : $(testdir)/%/run
	@echo "[Successfully completed test $(notdir $*)]"
	fusermount -q -u $(dir $@)mount || true
	touch $@ $(dir $@){run,prepare}

$(addsuffix /run,$(tests-base)) : $(testdir)/%/run : $(testdir)/%/prepare

$(addsuffix /prepare,$(tests-base)) : %/prepare : $(target) | $(testdir)
	@echo "[Preparing environment for test $(notdir $*)]"
	fusermount -q -u $(dir $@)mount || true
	rm -rf $(dir $@){data,mount}
	mkdir -p $(dir $@){data,mount}
	touch $@

$(testdir)/basic-listing/run:
	# prepare stage must be repeated both for every launch
	rm $(dir $@)prepare
	mkdir $(dir $@)data/dir1
	mkdir $(dir $@)data/dir2
	mkdir $(dir $@)data/dir2/subdir1
	echo "file1 content" > $(dir $@)data/dir1/file1
	echo "file2 content" > $(dir $@)data/dir1/file2
	echo "file3 content" > $(dir $@)data/dir2/subdir1/file3
	echo "file4 content" > $(dir $@)data/dir2/subdir1/file4
	setfattr -n user.xynta.tags -v 'tag1 tag2' $(dir $@)data/dir1/file2
	setfattr -n user.xynta.tags -v 'tag1' $(dir $@)data/dir2/subdir1/file3
	setfattr -n user.xynta.tags -v 'dir1 tag1 tag3' \
	    $(dir $@)data/dir2/subdir1/file4
	$(target) -d $(abspath $(dir $@)data) -m0 $(dir $@)mount
	test "$$(echo -e 'dir1\ndir2\nfile1\nfile2\nfile3\nfile4'; \
	    echo -e 'subdir1\ntag1\ntag2\ntag3')" = \
	    "$$(ls $(dir $@)mount)"
	test "$$(echo -e 'dir2\nfile1\nfile2\nfile4'; \
	    echo -e 'subdir1\ntag1\ntag2\ntag3')" = \
	    "$$(ls $(dir $@)mount/dir1)"
	test "$$(echo -e 'dir1\ndir2\nfile2\nfile3\nfile4'; \
	    echo -e 'subdir1\ntag2\ntag3')" = \
	    "$$(ls $(dir $@)mount/tag1)"
	# repeat same tests with default min-files
	fusermount -u $(dir $@)mount
	$(target) -d $(abspath $(dir $@)data) $(dir $@)mount
	test "$$(echo -e 'file1\nfile2\nfile3\nfile4')" = \
	    "$$(ls $(dir $@)mount)"
	test "$$(echo -e 'file1\nfile2\nfile4')" = "$$(ls $(dir $@)mount/dir1)"
	test "$$(echo -e 'file2\nfile3\nfile4')" = "$$(ls $(dir $@)mount/tag1)"
	# test file contents
	echo "file1 content" | diff - $(dir $@)mount/file1
	echo "file2 content" | diff - $(dir $@)mount/tag2/tag1/file2
	echo "file3 content" | diff - $(dir $@)mount/subdir1/file3
	echo "file4 content" | diff - $(dir $@)mount/dir1/tag3/file4
	touch $@

