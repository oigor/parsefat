CFLAGS =	-std=c11 -O0 -g -Wall -fmessage-length=0

OBJS =		parsefat.o disk_from_file.o fat.o

LIBS =

TARGET =	parsefat

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
