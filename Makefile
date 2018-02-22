NAME=mario-watch

GAME_C_FILES = main.c sdk/lib/blitter/blitter.c sdk/lib/blitter/blitter_sprites3.c
GAME_BINARY_FILES = data/*.spr data/game.map

DEFINES = VGA_MODE=400 VGA_BPP=8
SCRIPTS = sdk/lib/blitter/scripts/

.DELETE_ON_ERROR: data/%.spr

include sdk/kernel/bitbox.mk

main.c: game.h data.h

SPRITES := luigi mario status parcel truck
IMAGES  := info intro background

data/sprite_% : %.tsx # also makes data/%_xxx.spr
	@mkdir -p data
	$(SCRIPTS)/mk_spr.py $^ -o data -p MICRO
	touch $@

data/%.spr : %.png
	@mkdir -p data
	$(SCRIPTS)/mk_spr.py $^ -o data/$*.spr -p MICRO

game.h data/game.map: game.tmx
	@mkdir -p data
	$(SCRIPTS)/mk_tmap.py $^ > game.h
	@mv game.map data

data.h : $(SPRITES:%=data/sprite_%) $(IMAGES:%=data/%.spr) data/game.map

clean-assets:
	rm -f data/* data.h game.h sprite_*.h