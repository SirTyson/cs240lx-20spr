# change these so you can do a "make test" and it verifies what you already
# have in there.  (probably need a pi in there).   also you will need to 
# do a better design.
# ah: for test, doesn't work for us b/c we don't have the code...
#
# also it is tricky if they swap with our code and their code.
SUBDIRS=  scope test-gen

.PHONY: all check clean
all check clean: $(SUBDIRS)

all: TARGET=all
check: TARGET=check
clean: TARGET=clean

# No, you can't do TARGET=$@, or at least I don't know how to.

# recursive call to make
$(SUBDIRS): force
	$(MAKE) -s -C $@ $(TARGET)
run:
	pi-install /dev/ttyUSB1 ./scope/scope.bin
	pi-install /dev/ttyUSB0 ./test-gen/test-gen.bin

clean:
	rm -f *~ *.bak

.PHONY: force
	force :;
