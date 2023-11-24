# make              # to compile files and create the executables
# make pgm          # to download example images to the pgm/ dir
# make setup        # to setup the test files in test/ dir
# make tests        # to run basic tests
# make clean        # to cleanup object files and executables
# make cleanobj     # to cleanup object files only

CFLAGS = -Wall -O2 -g

PROGS = imageTool imageTest

TESTS = test1 test2 test3 test4 test5 test6 test7 test8 test9

# Default rule: make all programs
all: $(PROGS)

imageTest: imageTest.o image8bit.o instrumentation.o

imageTest.o: image8bit.h instrumentation.h

imageTool: imageTool.o image8bit.o instrumentation.o

imageTool.o: image8bit.h instrumentation.h

# Rule to make any .o file dependent upon corresponding .h file
%.o: %.h

pgm:
	wget -O- https://sweet.ua.pt/jmr/aed/pgm.tgz | tar xzf -

.PHONY: setup
setup: test/

test/:
	wget -O- https://sweet.ua.pt/jmr/aed/test.tgz | tar xzf -
	@#mkdir -p $@
	@#curl -s -o test/aed-trab1-test.zip https://sweet.ua.pt/mario.antunes/aed/test/aed-trab1-test.zip
	@#unzip -q -o test/aed-trab1-test.zip -d test/

test1_1: $(PROGS) setup
	./imageTool pgm/medium/ireland-03_640x480.pgm neg save neg2.pgm

test1: $(PROGS) setup
	./imageTool test/original.pgm neg save neg.pgm
	cmp neg.pgm test/neg.pgm

test2: $(PROGS) setup
	./imageTool test/original.pgm thr 128 save thr.pgm
	cmp thr.pgm test/thr.pgm

test2_1: $(PROGS) setup
	./imageTool pgm/medium/airfield-05_640x480.pgm thr 128 save thr2.pgm

test3_1: $(PROGS) setup
	./imageTool pgm/medium/tac-pulmao_512x512.pgm bri .33 save bri2.pgm
	cmp bri.pgm test/bri.pgm

test3: $(PROGS) setup
	./imageTool test/original.pgm bri .33 save bri.pgm

test4: $(PROGS) setup
	./imageTool test/original.pgm rotate save rotate.pgm
	cmp rotate.pgm test/rotate.pgm

test4_1: $(PROGS) setup
	./imageTool pgm/medium/tools_2_765x460.pgm rotate save rotate2.pgm

test5: $(PROGS) setup
	./imageTool test/original.pgm mirror save mirror.pgm
	cmp mirror.pgm test/mirror.pgm

test5_1: $(PROGS) setup
	./imageTool pgm/small/art4_300x300.pgm mirror save mirror2.pgm

test6: $(PROGS) setup
	./imageTool test/original.pgm crop 100,100,100,100 save crop.pgm
	cmp crop.pgm test/crop.pgm

test6_1: $(PROGS) setup
	./imageTool pgm/small/art4_300x300.pgm crop 100,100,100,100 save crop2.pgm

test7: $(PROGS) setup
	./imageTool test/small.pgm test/original.pgm paste 100,100 save paste.pgm
	cmp paste.pgm test/paste.pgm

test7_1: $(PROGS) setup
	./imageTool pgm/small/bird_256x256.pgm pgm/large/einstein_940x940.pgm paste 100,100 save paste2.pgm

test8: $(PROGS) setup
	./imageTool test/small.pgm test/original.pgm blend 100,100,.33 save blend.pgm
	cmp blend.pgm test/blend.pgm

test8_1: $(PROGS) setup
	./imageTool pgm/small/art3_222x217.pgm pgm/large/ireland-06-1200x1600.pgm blend 100,100,.33 save blend2.pgm

test9: $(PROGS) setup
	./imageTool test/original.pgm blur 7,7 save blur.pgm
	cmp blur.pgm test/blur.pgm

test10: $(PROGS) setup
	./imageTool test/crop.pgm test/original.pgm locate save locate.pgm

test9_1: $(PROGS) setup
	./imageTool pgm/medium/mandrill_512x512.pgm blur 7,7 save blur2.pgm

.PHONY: tests
tests: $(TESTS)

# Make uses builtin rule to create .o from .c files.

cleanobj:
	rm -f *.o

clean: cleanobj
	rm -f $(PROGS)

