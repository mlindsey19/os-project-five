#mitch lindsey
#$cs4760
#04-23-2019

CC = cc
CFLAGS = -g -Wall -lrt

OBJ1 = oss.o checkargs.o procComm.o
OBJ2 = user.o

TARGET1 = oss
TARGET2 = user


all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJ1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJ1)
$(TARGET2): $(OBJ2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJ2)

%.o: %.c *.h
	$(CC) $(CFLAGS) -c $*.c -o $*.o

clean:
	rm $(TARGET1) $(TARGET2) *.o output.txt
