# Makefile for appl and mmanage
.PHONY: clean 

CC	 = /usr/bin/gcc
CFLAGS	 = -Wall -std=c99 $(DFLAGS)

LDFLAGS =  

SRCDIR   = ./src
OBJDIR   = ./obj
BINDIR   = ./bin

EXEFILES     = socket_client socket_srv # Anwendungen
srcfiles     = $(wildcard $(SRCDIR)/*.c) # all src files
deps         = $(subst $(SRCDIR)/,$(OBJDIR)/,$(srcfiles:.c=.d))

all: $(patsubst %,$(BINDIR)/%,$(EXEFILES))


-include $(deps)

# Build all test programs

# calculate dependcies
$(OBJDIR)/%.d: $(SRCDIR)/%.c
	@echo "computing dependencies for  $<"
	@mkdir -p $(@D)
	@$(CC) -MM -MT "$@ $(patsubst %.d,%.o,$@)" -MF $@ $<

# compile a module
$(OBJDIR)/%.o: $(SRCDIR)/%.c 
	@echo "compiling $< ..."
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS)  -c $< -o $@

# link an executable
$(BINDIR)/% : $(OBJDIR)/%.o $(subst $(SRCDIR)/,$(OBJDIR)/,$(modulefiles:.c=.o)) 
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -r -f $(OBJDIR) $(BINDIR) 
	rm -rf *.o client server

# EOF
