#TC = msp430-
#TC = avr-
#TC = arm-none-eabi-
TC = 
CONSOLE_DEBUG = -DSMDNS_CONSOLE_DEBUG
CC = $(TC)gcc
AR = $(TC)ar
BIN = ../bin
CC_FLAGS = -std=c99 $(CONSOLE_DEBUG)

LIB = $(BIN)/libnnet.a

default: depend $(LIB)

depend:
	g++ $(CFLAGS) -MM $(SRC) >depend

SRC = nnet_buf.c nnet_config.c nnet_utils.c nnet_eth.c nnet_ip.c nnet_udp.c nnet_mdns.c
OBJ = $(SRC:.c=.o)

.c.o:
	$(CC) $(CC_FLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) cr $(LIB) $(OBJ)

clean:
	-rm *.o depend $(LIB)

-include depend
