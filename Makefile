# Makefile 

NAME=mario-watch

GAME_C_FILES = main.c lib/blitter/blitter.c lib/blitter/blitter_sprites.c

DEFINES = VGA_MODE=400 VGA_BPP=8
GAME_BINARY_FILES = data.bin


TMX = $(BITBOX)/lib/blitter/scripts/tmx2.py

include $(BITBOX)/kernel/bitbox.mk 

main.c: data.h game.h
	
data.h data.bin: data/game.tmap 
	python $(BITBOX)/scripts/mkdata.py > data.h

%.h data/%.tmap: %.tmx
	@mkdir -p data
	$(TMX) -o data -msi $< > $*.h

clean::
	rm -f data/* data.bin data.h game.h 