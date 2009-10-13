SRC = design

FIGS = OC-res-concurrency
FIGS += SR-OC-FSM-v2
FIGS += SR-UI-BPM
FIGS += SR-UI-OCM
FIGS += SR-view
FIGS += SR-MVC

FIG_FILES = $(FIGS:%=%.fig)

IMAGES = $(FIGS:%=%.png)

FIG2DEV = fig2dev -L png -b 10 -F -S 4
ASCIIDOC = asciidoc -a toc
DBLATEX = dblatex --pdf

all: images html pdf

images: $(IMAGES)

%.png: %.fig
	$(FIG2DEV) $^ $@ 

html: $(SRC).txt $(IMAGES)
	$(ASCIIDOC) -a asciimath -b xhtml11 $(SRC).txt
    
pdf: $(SRC).txt $(IMAGES)
	$(ASCIIDOC) -b docbook $(SRC).txt
	$(DBLATEX) $(SRC).xml
    
clean:
	$(RM) $(SRC).html $(SRC).xml $(SRC).pdf $(IMAGES)
