# Native Windows builds use PDCursesMod's VT backend.  MSYS2 UCRT64 is
# selected automatically; Unix/Mac retain the existing build verbatim.
# ROGUE_BUILD=windows|unix can override the automatic selection.
ifeq ($(strip $(ROGUE_BUILD)),)
ifeq ($(MSYSTEM),UCRT64)
ROGUE_BUILD := windows
else
ROGUE_BUILD := unix
endif
endif
ifeq ($(ROGUE_BUILD),windows)
include Makefile.win
else ifeq ($(ROGUE_BUILD),unix)
#
# Makefile for Rogue 5.2 macOS/Linux port
# Original Makefile: @(#)Makefile 4.13 (Berkeley) 1/23/82
#
# Rogue: Exploring the Dungeons of Doom
# Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
# All rights reserved.
#
# See LICENSE.TXT for copyright and licensing information.
#

DISTNAME = rogue52_005

HDRS = rogue.h extern.h jec/language.h jec/unicode_text.h jec/color.h windows/rogue_windows.h
# The source manifest is authoritative. Derive objects from it so the build
# and the source distribution cannot silently get out of sync.
CFILES = vers.c extern.c armor.c chase.c command.c daemon.c daemons.c \
	fight.c init.c io.c jec/language.c jec/unicode_text.c jec/color.c list.c main.c misc.c monsters.c move.c \
	new_level.c options.c pack.c passages.c potions.c rings.c rip.c \
	rooms.c save.c scrolls.c state.c sticks.c things.c weapons.c wizard.c \
	xcrypt.c mdport.c mach_dep.c
OBJS = $(CFILES:.c=.o)
MISC = .github/workflows/build.yml Makefile Makefile.win configure LICENSE.TXT README.md README_mac_linux_ja.md README_mac_linux_en.md README_windows_ja.md README_windows_en.md rogue.6 rogue.md rogue_jp.md rogue_en.txt rogue_ja.txt windows/rogue_windows.c windows/rogue.ico windows/rogue-icon.rc windows/README_binary_ja.txt windows/README_binary_en.txt
# Sort also removes duplicates if a file is added to more than one list.
DISTFILES = $(sort $(CFILES) $(HDRS) $(MISC))
DOCS = README.md README_mac_linux_ja.md README_mac_linux_en.md README_windows_ja.md README_windows_en.md LICENSE.TXT rogue.md rogue_jp.md rogue_en.txt rogue_ja.txt

-include config.mk

# Accept both executable-name interfaces in direct builds, as on Windows.
# PROGRAM takes precedence if both are supplied.  A configured build can
# instead use ./configure --program=NAME to regenerate its file-name defaults.
ifneq ($(filter command line,$(origin PROGRAM)),)
ifneq ($(strip $(PROGRAM)),)
override PROG := $(PROGRAM)
endif
endif

CC ?= clang
PROG ?= rogue
SAVEDIR ?= .
FILEDIR ?=
SAVEFILE ?= $(PROG).save
SCOREFILE ?= $(PROG).scr
EXTRA_CPPFLAGS ?=
DEFAULT_LANGUAGE ?= 0
DEFAULT_COLOR ?= 1
# Modern mode hides the cursor; make original shows it, like Rogue 5.4.5 JEC.
DEFAULT_CURSOR ?= 0
# Ordinary builds follow bends in passages; make original does not.
DEFAULT_PASSGO ?= 1
# Prompt for a call name after using an unidentified item by default.
DEFAULT_ASKME ?= 1
CFLAGS ?= -O2 -std=gnu89
LDFLAGS ?=
CRLIB ?= -lncurses

FILEDEFS = -DDEFAULT_SAVEDIR=\"$(SAVEDIR)\" \
	-DDEFAULT_SAVEFILE=\"$(SAVEFILE)\" \
	-DSCOREFILE=\"$(SCOREFILE)\"
CPPFLAGS += -I. $(FILEDEFS) -DDEFAULT_LANGUAGE=$(DEFAULT_LANGUAGE) -DDEFAULT_COLOR=$(DEFAULT_COLOR) -DDEFAULT_CURSOR=$(DEFAULT_CURSOR) -DDEFAULT_PASSGO=$(DEFAULT_PASSGO) -DDEFAULT_ASKME=$(DEFAULT_ASKME) $(EXTRA_CPPFLAGS)

PREFIX ?= /usr/local
prefix ?= $(PREFIX)
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin
datarootdir ?= $(prefix)/share
mandir ?= $(datarootdir)/man
docdir ?= $(datarootdir)/doc/$(PROG)

INSTALL ?= install
INSTALL_DIR ?= $(INSTALL) -d
INSTALL_PROGRAM ?= $(INSTALL) -m 755
INSTALL_DATA ?= $(INSTALL) -m 644
RM ?= rm -f
TAR ?= tar
GZIP ?= gzip

.PHONY: all japanese english original wizard install uninstall clean distclean dist.src show-config FORCE

all: $(PROG)

japanese:
	$(MAKE) DEFAULT_LANGUAGE=1 $(PROG)

english:
	$(MAKE) DEFAULT_LANGUAGE=2 $(PROG)

# "original" defaults to English, but an explicit command-line
# DEFAULT_LANGUAGE=1/0/2 still overrides that default.
ifeq ($(origin DEFAULT_LANGUAGE),command line)
ORIGINAL_LANGUAGE := $(DEFAULT_LANGUAGE)
else
ORIGINAL_LANGUAGE := 2
endif
ifeq ($(origin DEFAULT_COLOR),command line)
ORIGINAL_COLOR := $(DEFAULT_COLOR)
else
ORIGINAL_COLOR := 0
endif
# An explicit DEFAULT_CURSOR=0/1 on the command line overrides original mode.
ifeq ($(origin DEFAULT_CURSOR),command line)
ORIGINAL_CURSOR := $(DEFAULT_CURSOR)
else
ORIGINAL_CURSOR := 1
endif
# As with the other defaults, an explicit command-line value wins.
ifeq ($(origin DEFAULT_PASSGO),command line)
ORIGINAL_PASSGO := $(DEFAULT_PASSGO)
else
ORIGINAL_PASSGO := 0
endif
ifeq ($(origin DEFAULT_ASKME),command line)
ORIGINAL_ASKME := $(DEFAULT_ASKME)
else
ORIGINAL_ASKME := 0
endif
original:
	$(MAKE) DEFAULT_LANGUAGE=$(ORIGINAL_LANGUAGE) DEFAULT_COLOR=$(ORIGINAL_COLOR) DEFAULT_CURSOR=$(ORIGINAL_CURSOR) DEFAULT_PASSGO=$(ORIGINAL_PASSGO) DEFAULT_ASKME=$(ORIGINAL_ASKME) $(PROG)

wizard:
	$(MAKE) clean
	$(MAKE) EXTRA_CPPFLAGS="$(EXTRA_CPPFLAGS) -DWIZARD" $(PROG)

# Changes to build language, compiler or link settings must rebuild objects
# and the executable.  Compare using shell built-ins so even minimal macOS/
# Linux environments do not need an external cmp utility.
.build-settings: FORCE
	@printf '%s\n' '$(CC)|$(CFLAGS)|$(CPPFLAGS)|$(LDFLAGS)|$(CRLIB)' > .build-settings.tmp
	@{ IFS= read -r current < .build-settings.tmp; \
	   if test -r "$@" && IFS= read -r previous < "$@" && \
	      test "$$current" = "$$previous"; then \
	       rm -f .build-settings.tmp; \
	   else \
	       mv -f .build-settings.tmp "$@"; \
	   fi; }

# Rebuild objects when any shared header changes. The previous dependency
# existed only on the linked executable and could leave stale .o files.
$(OBJS): $(HDRS) .build-settings

$(PROG): $(HDRS) $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(CRLIB) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

show-config:
	@echo "CC=$(CC)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "CPPFLAGS=$(CPPFLAGS)"
	@echo "LDFLAGS=$(LDFLAGS)"
	@echo "CRLIB=$(CRLIB)"
	@echo "PROG=$(PROG)"
	@echo "SAVEDIR=$(SAVEDIR)"
	@echo "FILEDIR=$(FILEDIR)"
	@echo "SAVEFILE=$(SAVEFILE)"
	@echo "SCOREFILE=$(SCOREFILE)"
	@echo "DEFAULT_LANGUAGE=$(DEFAULT_LANGUAGE)"
	@echo "DEFAULT_COLOR=$(DEFAULT_COLOR)"
	@echo "DEFAULT_CURSOR=$(DEFAULT_CURSOR)"
	@echo "DEFAULT_PASSGO=$(DEFAULT_PASSGO)"
	@echo "DEFAULT_ASKME=$(DEFAULT_ASKME)"
	@echo "prefix=$(prefix)"
	@echo "bindir=$(bindir)"
	@echo "mandir=$(mandir)"
	@echo "docdir=$(docdir)"

install: all
	$(INSTALL_DIR) "$(DESTDIR)$(bindir)"
	$(INSTALL_PROGRAM) "$(PROG)" "$(DESTDIR)$(bindir)/$(PROG)"
	$(INSTALL_DIR) "$(DESTDIR)$(mandir)/man6"
	# Adapt only the installed man page's title, name and synopsis to PROG.
	# Keep the checked-in historical rogue.6 unchanged (like the guides).
	@sed \
	  -e '1s|^\.TH ROGUE 6|.TH $(PROG) 6|' \
	  -e '/^\.SH NAME/,/^\.SH SYNOPSIS/s|^rogue \\- Exploring|$(PROG) \\- Exploring|' \
	  -e '/^\.SH SYNOPSIS/,/^\.SH DESCRIPTION/s|^\.B rogue$$|.B $(PROG)|' \
	  rogue.6 > "$(DESTDIR)$(mandir)/man6/$(PROG).6"
	$(INSTALL_DIR) "$(DESTDIR)$(docdir)"
	for f in $(DOCS); do \
		$(INSTALL_DATA) "$$f" "$(DESTDIR)$(docdir)/$$f"; \
	done
	@if [ -n "$(FILEDIR)" ]; then \
		dir="$(FILEDIR)"; \
		case "$$dir" in \
			\~) dir="$$HOME" ;; \
			\~/*) dir="$$HOME/$${dir#\~/}" ;; \
		esac; \
		if [ "$$dir" != "." ]; then \
			case "$$dir" in \
				/*) target="$(DESTDIR)$$dir" ;; \
				*) target="$$dir" ;; \
			esac; \
			$(INSTALL_DIR) "$$target"; \
		fi; \
	fi

uninstall:
	$(RM) "$(DESTDIR)$(bindir)/$(PROG)"
	$(RM) "$(DESTDIR)$(mandir)/man6/$(PROG).6"
	for f in $(DOCS); do \
		$(RM) "$(DESTDIR)$(docdir)/$$f"; \
	done
	-rmdir "$(DESTDIR)$(docdir)" 2>/dev/null || true

clean:
	$(RM) .build-settings .build-settings.tmp $(OBJS) core a.out p.out $(PROG) rogue rogue52 make.out rogue.tar \
		rogue.tar.gz rogue.cat rogue.doc vgrind.* linterrs distmod.o

distclean: clean
	$(RM) config.mk config.log config.status conftest conftest.c conftest.err conftest.o

# Create a standalone source distribution without cleaning the working tree.
# COPYFILE_DISABLE suppresses macOS AppleDouble/xattr records in tar archives.
# All staging is done in a private temporary directory, never in the source.
dist.src:
	@set -eu; \
	umask 022; \
	tmp=$$(mktemp -d "$${TMPDIR:-/tmp}/$(DISTNAME)-dist.XXXXXX"); \
	trap 'rm -rf "$$tmp"' 0; \
	trap 'exit 1' 1 2 3 15; \
	stage="$$tmp/$(DISTNAME)-src"; \
	mkdir -p "$$stage"; \
	export COPYFILE_DISABLE=1; \
	for file in $(DISTFILES); do \
	    test -f "$$file" || { echo "dist.src: missing file: $$file" >&2; exit 1; }; \
	    mkdir -p "$$stage/$$(dirname "$$file")"; \
	    cp "$$file" "$$stage/$$file"; \
	done; \
	chmod 755 "$$stage/configure"; \
	$(TAR) -cf "$$tmp/$(DISTNAME)-src.tar" -C "$$tmp" "$(DISTNAME)-src"; \
	$(GZIP) -f "$$tmp/$(DISTNAME)-src.tar"; \
	mv -f "$$tmp/$(DISTNAME)-src.tar.gz" "$(DISTNAME)-src.tar.gz"; \
	echo "Created $(DISTNAME)-src.tar.gz"

else
$(error Unsupported ROGUE_BUILD=$(ROGUE_BUILD); choose windows or unix)
endif
