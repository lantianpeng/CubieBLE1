CC = gcc

INCLUDE = -I../crc32/ -I../crypt -I../protobuf
CFLAGS = -std=c99 -Wall $(INCLUDE) -g
SOURCES = ../crc32/crc32.c \
		  ../crypt/aes_crypt.c \
		  ../crypt/aes/aes_cbc.c \
		  ../crypt/aes/aes_core.c \
		  ../crypt/aes/cbc128.c \
		  ../protobuf/epb.c \
		  ../protobuf/epb_MmBp.c \
		  ../protobuf/epb_WristBand.c


OBJS = $(addsuffix .o, $(basename $(SOURCES)))

all: crc32_example aes_example pb_example wristband_example wristband_check

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

crc32_example: crc32_example.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

aes_example: aes_example.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

pb_example: pb_example.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

wristband_example: wristband_example.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

wristband_check: wristband_check.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf *.exe *.o
	rm -rf ../crc32/*.o
	rm -rf ../crypt/*.o
	rm -rf ../crypt/aes/*.o
	rm -rf ../protobuf/*.o
