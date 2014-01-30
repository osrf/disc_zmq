#TC = msp430-
#TC = avr-
#TC = arm-none-eabi-
TC = 
CONSOLE_DEBUG = -DNNET_CONSOLE_DEBUG
CC = $(TC)gcc
AR = $(TC)ar
BIN = 
CC_FLAGS = -std=c99 $(CONSOLE_DEBUG) -I../libnnet -L../bin -lnnet
LIBS = -l../lib/libnnet.a
TESTS = test_mdns test_udp

default: $(TESTS)

TEST_SRC = $(TESTS:=.c)

#.c.:
#	$(CC) $(CC_FLAGS) -c $< -o $@

$(TESTS):
	@echo $@ 
	$(CC) $(CC_FLAGS) -o $@ $@.c common.c -lnnet


#$(LIB): $(OBJ)
#	$(AR) cr $(LIB) $(OBJ)

clean:
	#-rm *.o depend $(LIB)

