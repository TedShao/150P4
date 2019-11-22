# Target library
lib := libuthread.a
objs := disk.o fs.o

CC := gcc
CFLAGS := -Wall -Werror
CFLAGS += -g

all: $(lib)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

$(lib): $(objs)
	ar rcs $@ $^

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "CLEAN"
	$(Q)rm -f $(lib) $(objs) $(deps)
