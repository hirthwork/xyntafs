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
target = $(builddir)/xynta
headers = $(shell find src -type f -name \*.hpp)
sources = $(shell find src -type f -name \*.cpp)

tests = basic-listing errors
testdir = $(builddir)/test
tests-base = $(addprefix $(testdir)/,$(tests))
tests-done = $(addsuffix /done,$(tests-base))

.PHONY: clean test full-test

$(target): Makefile $(headers) $(sources) | $(builddir)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(sources) $(LDFLAGS) -o $@

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
	    /bin/echo -e "\n[Running tests for $$x]"; \
	    $(MAKE) CXX=$$x builddir=$(builddir)/$$x test || exit 1; \
	done

test: $(tests-done)

$(tests-done) : $(testdir)/%/done : $(testdir)/%/run
	@/bin/echo -e "\n[Successfully completed test $(notdir $*)]"
	fusermount -z -q -u $(dir $@)mount || true
	touch $@ $(dir $@)run $(dir $@)prepare

$(addsuffix /run,$(tests-base)) : $(testdir)/%/run : $(testdir)/%/prepare

$(addsuffix /prepare,$(tests-base)) : %/prepare : $(target) | $(testdir)
	@/bin/echo -e "\n[Preparing environment for test $(notdir $*)]"
	fusermount -z -q -u $(dir $@)mount || true
	rm -rf $(dir $@)data $(dir $@)mount
	mkdir -p $(dir $@)data $(dir $@)mount
	touch $@

$(testdir)/basic-listing/run:
	# prepare stage must be repeated both for every launch
	rm $(dir $@)prepare
	mkdir $(dir $@)data/dir1
	mkdir $(dir $@)data/dir2
	mkdir $(dir $@)data/dir2/subdir1
	/bin/echo "file1 content" > $(dir $@)data/dir1/file1.xml
	/bin/echo "file2 content" > $(dir $@)data/dir1/file2
	/bin/echo "file3 content" > $(dir $@)data/dir2/subdir1/file3
	/bin/echo "file4 content" > $(dir $@)data/dir2/subdir1/file4
	setfattr -n user.xynta.tags -v 'tag1 tag2' $(dir $@)data/dir1/file2
	setfattr -n user.xynta.tags -v 'tag1' $(dir $@)data/dir2/subdir1/file3
	setfattr -n user.xynta.tags -v 'dir1 tag1 tag3 multi\ w\\\\rd' \
	    $(dir $@)data/dir2/subdir1/file4
	$(target) -d $(abspath $(dir $@)data) -m0 -- $(dir $@)mount
	test "$$(/bin/echo -e 'dir1\ndir2\nfile1.xml\nfile2\nfile3\nfile4'; \
	    /bin/echo -e 'multi w\\rd\nsubdir1\ntag1\ntag2\ntag3\nxml')" = \
	    "$$(ls $(dir $@)mount)"
	test "$$(/bin/echo -e 'dir2\nfile1.xml\nfile2\nfile4'; \
	    /bin/echo -e 'multi w\\rd\nsubdir1\ntag1\ntag2\ntag3\nxml')" = \
	    "$$(ls $(dir $@)mount/dir1)"
	test "$$(/bin/echo -e 'dir1\ndir2\nfile2\nfile3\nfile4'; \
	    /bin/echo -e 'multi w\\rd\nsubdir1\ntag2\ntag3')" = \
	    "$$(ls $(dir $@)mount/tag1)"
	# repeat same tests with default min-files and single thread
	fusermount -z -u $(dir $@)mount
	$(target) -d $(abspath $(dir $@)data) -- -s $(dir $@)mount
	test "$$(/bin/echo -e 'file1.xml\nfile2\nfile3\nfile4')" = \
	    "$$(ls $(dir $@)mount)"
	test "$$(/bin/echo -e 'file1.xml\nfile2\nfile4')" = \
	    "$$(ls $(dir $@)mount/dir1)"
	test "$$(/bin/echo -e 'file2\nfile3\nfile4')" = \
	    "$$(ls $(dir $@)mount/tag1)"
	# test file contents
	/bin/echo "file1 content" | diff - $(dir $@)mount/file1.xml
	/bin/echo "file1 content" | diff - $(dir $@)mount/xml/file1.xml
	/bin/echo "file2 content" | diff - $(dir $@)mount/tag2/tag1/file2
	/bin/echo "file3 content" | diff - $(dir $@)mount/subdir1/file3
	/bin/echo "file4 content" | diff - $(dir $@)mount/dir1/tag3/file4
	/bin/echo "file4 content" | diff - $(dir $@)mount/multi\ w\\rd/file4
	touch $@

$(testdir)/errors/run:
	rm $(dir $@)prepare
	/bin/echo "file1 content" > $(dir $@)data/file1
	setfattr -n user.xynta.tags -v tag1 $(dir $@)data/file1
	/bin/echo "file2 content" > $(dir $@)data/file2
	setfattr -n user.xynta.tags -v tag2 $(dir $@)data/file2
	# create file/tag name collision
	/bin/echo "file3 content" > $(dir $@)data/tag1
	echo '! $(target) -d $(abspath $(dir $@)data) -m0 -- $(dir $@)mount' \
	    | sh
	rm $(dir $@)data/tag1
	# create file/tag self collision
	/bin/echo "file4 content" > $(dir $@)data/file4
	setfattr -n user.xynta.tags -v file4 $(dir $@)data/file4
	echo '! $(target) -d $(abspath $(dir $@)data) -m0 -- $(dir $@)mount' \
	    | sh
	rm $(dir $@)data/file4
	# create file names collision
	mkdir $(dir $@)data/dir
	/bin/echo "file5 content" > $(dir $@)data/dir/file1
	echo '! $(target) -d $(abspath $(dir $@)data) -m0 -- $(dir $@)mount' \
	    | sh
	rm -r $(dir $@)data/dir
	# try load data from non-existing dir
	echo '! $(target) -d $(abspath $(dir $@)ndata) -m0 -- $(dir $@)mount' \
	    | sh
	# pass invalid option
	echo '! $(target) -d $(abspath $(dir $@)ndata) -X -- $(dir $@)mount' \
	    | sh
	# request help
	$(target) -h
	# `forget' to pass data dir
	echo '! $(target) -- $(dir $@)mount' | sh
	# ok, test runtime errors on invalid paths
	$(target) -d $(abspath $(dir $@)data) -m0 -- $(dir $@)mount
	test ! -d $(dir $@)mount/tag1/tag2
	test ! -f $(dir $@)mount/tag1/file2
	test ! -e $(dir $@)mount/something
	# let's remove file and try access it
	test -f $(dir $@)mount/tag1/file1
	rm $(dir $@)data/file1
	test ! -f $(dir $@)mount/tag1/file1
	# let's try to append something to file
	echo '! echo >> $(dir $@)mount/file2' | sh
	touch $@

