CFLAGS = -O2 -pipe -flto
override CFLAGS += -Wall -Wextra -Wpedantic -Werror -Wstrict-overflow=5
override CFLAGS += $(shell pkg-config fuse --cflags)
override CFLAGS += -DFUSE_USE_VERSION=26 -Isrc

GCC_VERSION = \
    $(shell $(CXX) --version | grep -q '^g++' && \
	$(CXX) -dumpversion | cut -d. -f1)
OBSOLETE_COMPILER = \
    $(shell [ -n "$(GCC_VERSION)" ] && [ "$(GCC_VERSION)" -lt 6 ] && echo true)

ifeq ($(OBSOLETE_COMPILER),true)
    override CXXFLAGS += -std=c++1y -Dshared_mutex=shared_timed_mutex
else
    override CXXFLAGS += -std=c++1z
endif

override LDFLAGS += $(shell pkg-config fuse --libs)
override LDFLAGS += $(findstring -flto,$(CFLAGS) $(CXXFLAGS))
override LDFLAGS += $(findstring --coverage,$(CFLAGS) $(CXXFLAGS))

OPTIONS = $(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS)

COMPILERS = g++-4.9.3 g++-5.4.0 g++-6.2.0 clang++

BUILDDIR = build
XYNTA = $(BUILDDIR)/xynta

TESTS = basic-listing long-listing errors
TESTSDIR = $(BUILDDIR)/test
TESTS_BASE = $(addprefix $(TESTSDIR)/,$(TESTS))
TESTS_DONE = $(addsuffix /done,$(TESTS_BASE))
TESTS_RUN = $(addsuffix /run,$(TESTS_BASE))
TESTS_PREPARE = $(addsuffix /prepare,$(TESTS_BASE))

SRCS = $(shell find src -type f -name \*.cpp)
OBJS = $(patsubst src/%,$(BUILDDIR)/%,$(SRCS:.cpp=.o))
DEPS = $(OBJS:.o=.d)
OPTS = $(BUILDDIR)/build-options

.PHONY: clean test full-test force

$(XYNTA): $(OBJS)
	$(CXX) $^ $(LDFLAGS) -o $@.T
	mv $@.T $@

$(OBJS): $(BUILDDIR)/%.o: src/%.cpp $(BUILDDIR)/%.d Makefile $(OPTS)
	mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $(@:.o=.To) \
	    -MMD -MP -MF $(@:.o=.Td) -MT '$(BUILDDIR)/$*.o'
	mv $(@:.o=.Td) $(@:.o=.d)
	mv $(@:.o=.To) $@
	touch $@

$(DEPS): ;

-include $(DEPS)

$(OPTS): force | $(BUILDDIR)
	@echo $(OPTIONS) | cmp -s - $@ || \
	    (echo $(OPTIONS) > $@.T && mv $@.T $@)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	@for mountpoint in $(TESTS_BASE); do \
	    [ -d $$mountpoint/mount ] && \
		fusermount -z -q -u $$mountpoint/mount || true; \
	done
	@rm -rf $(BUILDDIR)

full-test:
	@for x in $(COMPILERS); do \
	    /bin/echo -e "\n[Running tests for $$x]"; \
	    $(MAKE) CXX=$$x BUILDDIR=$(BUILDDIR)/$$x test || exit 1; \
	done

test: $(TESTS_DONE)

$(TESTS_DONE): $(TESTSDIR)/%/done: $(TESTSDIR)/%/run
	@/bin/echo -e "\n[Successfully completed test $(notdir $*)]"
	@fusermount -z -q -u $(dir $@)mount || true
	@touch $@ $(dir $@)run $(dir $@)prepare

$(TESTS_RUN): $(TESTSDIR)/%/run: $(TESTSDIR)/%/prepare

$(TESTS_PREPARE): %/prepare: $(XYNTA)
	@/bin/echo -e "\n[Preparing environment for test $(notdir $*)]"
	@fusermount -z -q -u $(dir $@)mount || true
	@rm -rf $(dir $@)data $(dir $@)mount
	@mkdir -p $(dir $@)data $(dir $@)mount
	@touch $@

$(TESTSDIR)/basic-listing/run:
	# prepare stage must be repeated for incompleted every launch
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
	$(XYNTA) -d $(abspath $(dir $@)data) -- $(dir $@)mount
	test "$$(/bin/echo -e 'dir1\ndir2\nfile1.xml\nfile2\nfile3\nfile4'; \
	    /bin/echo -e 'multi w\\rd\nsubdir1\ntag1\ntag2\ntag3\nxml')" = \
	    "$$(ls $(dir $@)mount)"
	test "$$(/bin/echo -e 'dir2\nfile1.xml\nfile2\nfile4'; \
	    /bin/echo -e 'multi w\\rd\nsubdir1\ntag1\ntag2\ntag3\nxml')" = \
	    "$$(ls $(dir $@)mount/dir1)"
	test "$$(/bin/echo -e 'dir1\ndir2\nfile2\nfile3\nfile4'; \
	    /bin/echo -e 'multi w\\rd\nsubdir1\ntag2\ntag3')" = \
	    "$$(ls $(dir $@)mount/tag1)"
	# test that non-significant tags not eliminated
	test "$$(/bin/echo -e 'dir1\nfile3\nfile4\nmulti w\\rd'; \
	    /bin/echo -e 'subdir1\ntag1\ntag3')" = \
	    "$$(ls $(dir $@)mount/dir2)"
	test "$$(/bin/echo -e 'dir1\nfile3\nfile4\nmulti w\\rd'; \
	    /bin/echo -e 'tag1\ntag3')" = \
	    "$$(ls $(dir $@)mount/dir2/subdir1)"
	# test files content
	/bin/echo "file1 content" | cmp - $(dir $@)mount/file1.xml
	/bin/echo "file1 content" | cmp - $(dir $@)mount/xml/file1.xml
	/bin/echo "file2 content" | cmp - $(dir $@)mount/tag2/tag1/file2
	/bin/echo "file3 content" | cmp - $(dir $@)mount/subdir1/file3
	/bin/echo "file4 content" | cmp - $(dir $@)mount/dir1/tag3/file4
	/bin/echo "file4 content" | cmp - $(dir $@)mount/multi\ w\\rd/file4
	# run same tests with single thread and relative path
	fusermount -z -u $(dir $@)mount
	$(XYNTA) -d $(dir $@)data -- -s $(dir $@)mount
	# test file content again
	/bin/echo "file1 content" | cmp - $(dir $@)mount/file1.xml
	/bin/echo "file1 content" | cmp - $(dir $@)mount/xml/file1.xml
	touch $@

$(TESTSDIR)/long-listing/run:
	rm $(dir $@)prepare
	seq 1000 | while read x; do \
	    /bin/echo "file #$$x" > $(dir $@)data/file$$x; \
	done
	$(XYNTA) -d $(dir $@)data -- $(dir $@)mount
	# check all files present
	test 1000 -eq "$$(ls $(dir $@)mount | wc -l)"
	# check first five files
	test "$$(/bin/echo -e 'file1\nfile10\nfile100\nfile1000\nfile101')" = \
	    "$$(ls $(dir $@)mount | head -n5)"
	# check last four files
	test "$$(/bin/echo -e 'file996\nfile997\nfile998\nfile999')" = \
	    "$$(ls $(dir $@)mount | tail -n4)"
	# check not so last three files
	test "$$(/bin/echo -e 'file909\nfile91\nfile910')" = \
	    "$$(ls $(dir $@)mount | tail -n100 | head -n3)"
	# check content
	/bin/echo "file #777" | cmp - $(dir $@)mount/file777
	touch $@

$(TESTSDIR)/errors/run:
	rm $(dir $@)prepare
	/bin/echo "file1 content" > $(dir $@)data/file1
	setfattr -n user.xynta.tags -v tag1 $(dir $@)data/file1
	/bin/echo "file2 content" > $(dir $@)data/file2
	setfattr -n user.xynta.tags -v tag2 $(dir $@)data/file2
	# create file/tag name collision
	/bin/echo "file3 content" > $(dir $@)data/tag1
	echo '! $(XYNTA) -d $(dir $@)data -- $(dir $@)mount' | sh
	rm $(dir $@)data/tag1
	# create file/tag self collision
	/bin/echo "file4 content" > $(dir $@)data/file4
	setfattr -n user.xynta.tags -v file4 $(dir $@)data/file4
	echo '! $(XYNTA) -d $(dir $@)data -- $(dir $@)mount' | sh
	rm $(dir $@)data/file4
	# create file names collision
	mkdir $(dir $@)data/dir
	/bin/echo "file5 content" > $(dir $@)data/dir/file1
	echo '! $(XYNTA) -d $(dir $@)data -- $(dir $@)mount' | sh
	rm -r $(dir $@)data/dir
	# create broken symlink
	ln -s nowhere-is-here $(dir $@)data/file6
	echo '! $(XYNTA) -d $(dir $@)data -- $(dir $@)mount' | sh
	rm $(dir $@)data/file6
	# create pipe
	mknod $(dir $@)data/fipe p
	echo '! $(XYNTA) -d $(dir $@)data -- $(dir $@)mount' | sh
	rm $(dir $@)data/fipe
	# try load data from non-existing dir
	echo '! $(XYNTA) -d $(dir $@)ndata -- $(dir $@)mount' | sh
	# pass invalid option
	echo '! $(XYNTA) -d $(dir $@)data -X -- $(dir $@)mount' | sh
	# request help
	$(XYNTA) -h
	# `forget' to pass data dir
	echo '! $(XYNTA) -- $(dir $@)mount' | sh
	# ok, test runtime errors on invalid paths
	$(XYNTA) -d $(dir $@)data -- $(dir $@)mount
	test ! -d $(dir $@)mount/tag1/tag2
	test ! -f $(dir $@)mount/tag1/file2
	test ! -e $(dir $@)mount/something
	# duplicated tags in path are forbidden
	test -f $(dir $@)mount/tag1/file1
	test ! -f $(dir $@)mount/tag1/tag1/file1
	# let's remove file and try access it
	rm $(dir $@)data/file1
	test ! -f $(dir $@)mount/tag1/file1
	# let's try to append something to file
	echo '! echo >> $(dir $@)mount/file2' | sh
	touch $@

