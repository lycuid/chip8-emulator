include ../config.mk

IDIR:=.
ODIR:=$(BUILD)/cache
BIN:=$(BUILD)/lib$(NAME).a

SRCS:=$(IDIR)/$(NAME).c
OBJS:=$(SRCS:$(IDIR)/%.c=$(ODIR)/%.o)

CFLAGS:=$(CFLAGS) -I$(IDIR)

$(BIN): $(OBJS) ; @mkdir -p $(@D)
	$(AR) rcs $@ $^

$(ODIR)/%.o: $(IDIR)/%.c $(IDIR)/%.h ; @mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -fPIC -o $@ $<

.PHONY: run debug clean compile_flags fmt
clean: ; rm -rf $(BUILD)
compile_flags: ; @echo $(CFLAGS) | xargs -n1 > compile_flags.txt
