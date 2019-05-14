#MAKEFILE
.PHONY: clean
CC  = gcc
CFLAGS  = -Wall -g -O0
LDFLAGS = -g -O0

SRCDIR = src
OBJDIR = obj
BINDIR = bin
EXEFILE = chatprogram

srcfiles = $(wildcard $(SRCDIR)/*.c)
objfiles = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(srcfiles:.c=.o))
deps = $(objfiles:.o=.d)

all: $(BINDIR)/$(EXEFILE)
-include $(deps)

$(OBJDIR)/%.d: $(SRCDIR)/%.c
	@echo "Calculating dependencies for $<"
	@mkdir -p $(@D)
	@$(CC) -MM -MT "$@ $(patsubst %.d,%.o,$@)" -MF $@ $<
	
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "(Cygwin): Compiling $<..."
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	
$(BINDIR)/$(EXEFILE): $(objfiles)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $^
	
clean:
	rm -r -f $(OBJDIR) $(BINDIR)