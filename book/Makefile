# Build the book: two LuaLaTeX passes, the second for the contents and the references, and a third
# when the second moved a page number the references had already used.
#
#   make                                     build embedded-c.pdf, dated today
#   make VERSION=book-v2                     the same, with the release's version on the title page
#   make partial ONLY=chapters/03/chapter    only the parts named (comma-separated), into
#                                            $(BUILD)/book.pdf; references into the rest are left
#                                            undefined, so none of them is an error
#   make clean                               remove everything the build writes
#
# BUILD may be set to build somewhere other than book/build, so two builds can run side by side.
#
# The demos' source files, the Makefile included, and the course's circuit diagrams and screenshots
# are typeset straight from their files under lectures/, so a change to one of those is a change to
# the book on the next build.

ROOT    := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
BOOK    := $(ROOT)/book
BUILD   ?= $(BOOK)/build
VERSION ?=
ONLY    ?=
LATEX   := lualatex -interaction=nonstopmode -halt-on-error -file-line-error \
           -output-directory=$(BUILD)
# The book, with its version defined before anything is read; -jobname keeps the output book.pdf.
BOOKTEX := $(LATEX) -jobname=book '\def\bookversion{$(VERSION)}$(if $(ONLY),\includeonly{$(ONLY)})\input{book.tex}'

SOURCES := book.tex cbook.sty cbook.lua \
           $(wildcard front/*.tex chapters/*/*.tex back/*.tex back/*/*.tex)
# Every \include writes an .aux of its own, beside where the part lives, under $(BUILD).
PARTS   := $(sort $(dir $(wildcard chapters/*/chapter.tex back/*/*.tex)))
LECTURE := $(shell cd $(ROOT) && git ls-files 'lectures/*.c' 'lectures/*.h' 'lectures/*Makefile' \
             'lectures/*.png' 'lectures/*.jpg' | sed 's|^|$(ROOT)/|')

.PHONY: all partial clean FORCE

all: embedded-c.pdf

# The version the last build was made for. The file is rewritten only when VERSION changes, so a
# new version, or none after one, rebuilds the title page and nothing else does.
$(BUILD)/version: FORCE
	@mkdir -p $(addprefix $(BUILD)/,$(PARTS))
	@echo '$(VERSION)' | cmp -s - $@ || echo '$(VERSION)' > $@

embedded-c.pdf: $(SOURCES) $(LECTURE) $(BUILD)/version
	cd $(BOOK) && $(BOOKTEX) >/dev/null || (tail -40 $(BUILD)/book.log; exit 1)
	cd $(BOOK) && $(BOOKTEX) >/dev/null || (tail -40 $(BUILD)/book.log; exit 1)
	@! grep -q 'Rerun to get cross-references right' $(BUILD)/book.log || \
	    (cd $(BOOK) && $(BOOKTEX) >/dev/null) || (tail -40 $(BUILD)/book.log; exit 1)
	@grep -E "^(Overfull|Underfull)|Warning" $(BUILD)/book.log | sort | uniq -c | sort -rn | head -20 || true
	@! grep -q "There were undefined references" $(BUILD)/book.log || \
	    (echo "error: undefined references; see $(BUILD)/book.log"; exit 1)
	cp $(BUILD)/book.pdf $@

# A part of the book on its own, for working on one chapter: the same two passes, no check for
# undefined references, and nothing copied out of $(BUILD).
partial: FORCE
	@test -n '$(ONLY)' || (echo "error: name the parts to build, e.g. ONLY=chapters/03/chapter"; exit 1)
	@mkdir -p $(addprefix $(BUILD)/,$(PARTS))
	cd $(BOOK) && $(BOOKTEX) >/dev/null || (tail -40 $(BUILD)/book.log; exit 1)
	cd $(BOOK) && $(BOOKTEX) >/dev/null || (tail -40 $(BUILD)/book.log; exit 1)
	@grep -E "^(Overfull|Underfull)|Warning" $(BUILD)/book.log | grep -v 'Reference.*undefined' | \
	    sort | uniq -c | sort -rn | head -20 || true

clean:
	rm -rf $(BUILD) embedded-c.pdf
