

TGTBIN := mm

SRCS := mm.c utils.c
OBJS := $(SRCS:.c=.o)

CC := gcc
CFLAGS := -Wall 
INCLUDES := -Imicromouse_maze_tool-master/mazefiles/cfiles
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

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o $(TGTBIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

