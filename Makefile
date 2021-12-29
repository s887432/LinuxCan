CC = arm-buildroot-linux-gnueabi-gcc

OBJS = canrecv.o
#LDFLAGS = -lpthread

TARGET=canRecv

all: $(TARGET)
	
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf canRecv
	rm -rf $(OBJS)
