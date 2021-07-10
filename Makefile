

TGTBIN := mm
BUILDDIR := build

SRCS := mm.c utils.c
OBJS = $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

CC := gcc
CFLAGS := -Wall 
INCLUDES := 
LIBS := -lncurses
LFLAGS := 

.PHONY: debug release depend clean
debug: $(TGTBIN)
	@echo Debug build...

release: $(TGTBIN)
	@echo Release build...

$(TGTBIN): $(OBJS)
	@echo Linking target binary...
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TGTBIN) $(OBJS) $(LFLAGS) $(LIBS)

$(BUILDDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILDDIR)/%.d: %.c
	$(CC) -MM -MT $@ $(CFLAGS) $(INCLUDES) $< -o $@

-include $(DEPS)

clean:
	rm -f $(BUILDDIR)/*.o $(TGTBIN)

tar:
	tar cvf mm.tar.gz ./ -v '*build*' '*mm*' '*tags*'


