include config.mk

IDIR:=src
ODIR:=$(BUILD)/cache
BIN:=$(BUILD)/bin/$(NAME)

SRCS:=$(IDIR)/main.c
OBJS:=$(SRCS:$(IDIR)/%.c=$(ODIR)/%.o)

PKGS:=sdl2
CFLAGS:=$(CFLAGS) -I$(IDIR) -Ilib $(shell pkg-config --cflags $(PKGS))
LDFLAGS:=$(shell pkg-config --libs $(PKGS))

$(BIN): lib $(OBJS) ; @mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(filter-out $(firstword $^),$^) -Llib/$(BUILD) -l$(NAME)

.PHONY: lib
lib: ; $(MAKE) -C lib

$(ODIR)/%.o: $(IDIR)/%.c $(IDIR)/%.h ; @mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<
$(ODIR)/%.o: $(IDIR)/%.c ; @mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: run debug clean compile_flags fmt
run: $(BIN) ; ./$(BIN)
debug: $(BIN) ; lldb $(BIN)
clean: ; rm -rf $(BUILD) && $(MAKE) -C lib $@
compile_flags: ; @echo $(CFLAGS) | xargs -n1 > compile_flags.txt && $(MAKE) -C lib $@
fmt: ; git ls-files | grep -E '\.[ch]$$' | xargs -i clang-format -i {}
