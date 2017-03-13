# Makefile 

NAME=mario-watch

GAME_C_FILES = main.c lib/blitter/blitter.c lib/blitter/blitter_sprites.c

DEFINES = VGA_MODE=400 VGA_BPP=8

TMX2 = $(BITBOX)/lib/blitter/scripts/tmx2.py
TSX  = $(BITBOX)/lib/blitter/scripts/tsx.py

.DELETE_ON_ERROR:

include $(BITBOX)/kernel/bitbox.mk 

main.c: data.h game.h

SPRITES := luigi mario status parcel truck 
IMAGES  := info intro background

data/%.spr sprite_%.h : %.tsx
	@mkdir -p data
	$(TSX) $^ -mo data > sprite_$*.h

game.h $(IMAGES:%=data/game_%.spr) : game.tmx
	@mkdir -p data
	$(TMX2) -o data -m $< > game.h

data.h : $(SPRITES:%=data/%.spr) $(IMAGES:%=data/game_%.spr)
	python $(BITBOX)/lib/resources/embed.py $^ > data.h

clean::
	rm -f data/* data.h game.h sprite_*.h