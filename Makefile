TARGET = simulator
OBJS += linked_list.o
OBJS += schedulerFCFS.o
OBJS += schedulerLCFS.o
OBJS += schedulerSJF.o
OBJS += schedulerMLFQ.o
OBJS += scheduler.o
OBJS += simulator.o
OBJS += trace.o
OBJS += main.o
LIBS += -lm

TEST = linked_list_test
TEST_OBJS += linked_list.o
TEST_OBJS += linked_list_test.o

CC = /usr/bin/gcc
CFLAGS += -MMD -MP # dependency tracking flags
CFLAGS += -I./
CFLAGS += -std=gnu11 -g -Wall -Werror -Wconversion -Wno-unused-variable
LDFLAGS += $(LIBS)

all: CFLAGS += -O2 # release flags
all: $(TARGET) $(TEST)

release: clean all

debug: CFLAGS += -O0 # debug flags
debug: clean $(TARGET) $(TEST)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST): $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

DEPS = $(OBJS:%.o=%.d)
-include $(DEPS)

TEST_DEPS = $(TEST_OBJS:%.o=%.d)
-include $(TEST_DEPS)

clean:
	-@rm -r $(TARGET) $(TEST) $(OBJS) $(TEST_OBJS) $(DEPS) $(TEST_DEPS) sandbox 2> /dev/null || true

test:
	@chmod +x grade.py
	@sed -i -e 's/\r$$//g' *.py # dos to unix
	@sed -i -e 's/\r/\n/g' *.py # mac to unix
	./grade.py
