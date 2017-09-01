/* Mario Watch */


#include <stdint.h>
#include <stdbool.h>

#define NBPARCELS 6
#define PARCEL_MOVESPEED 30
#define TRUCK_MOVESPEED 20




#include "bitbox.h"
#include "lib/blitter/blitter.h"

// Resources handling  ---------------------------------
#include "lib/blitter/mapdefs.h"

#define DATA_IMPLEMENTATION
#include "data.h"
#undef DATA_IMPLEMENTATION


#define SPRITE_IMPLEMENTATION
#define MAPS_IMPLEMENTATION
#include "game.h"
#undef SPRITE_IMPLEMENTATION
#undef MAPS_IMPLEMENTATION

struct ExtSprite {
	object *sprite;
	uint8_t sprite_id; 
	uint8_t state;

	uint8_t frame;
	uint8_t waitframe; // frames until next frame, 0 if not
};

uint16_t last_buttons_state; // last state of gameoad

#define PRESSED(x) ((gamepad_buttons[0]&~last_buttons_state) == gamepad_##x)

// -- game states

// splash screen
void game_intro();

// instructions
void game_instr_down();
void game_instr_up();
void game_instr_wait();

// in level
void game_level_pre(); // starting game.
void game_level(); // in level
void game_rest(); // truck is leaving
void game_loose(); // lost one live
void game_over();
void game_pause();

void truck_launch();
void truck_move();

void enter_loose();
void enter_level();

// --- global vars

void (*frame_cb)(); // next callback.

object *o_bg, *o_intro, *o_info, *o_instr, *o_bottom;

struct ExtSprite parcels[NBPARCELS];
struct ExtSprite mario, luigi, truck, miss;

int score, nbparcels;
bool mario_lost; // tracks who lost 

// one map object per state for every sprite in this game

#define NBSPRITES 5 // luigi, parcel, mario, status, truck
#define MAXSTATEID 10 // maximum nb of states in all sprites (here from truck)
const struct MapObjectDef *map_objects[NBSPRITES][MAXSTATEID]; // sprite / state -> map object


// ----

void sprite_transfer(struct ExtSprite es)
{
	// sprite 255 means hidden
	if (es.state==255) {
		es.sprite->y=9999;
	} else {		
		const struct MapObjectDef *mod = map_objects[es.sprite_id][es.state];
		es.sprite->x = mod->x;
		es.sprite->y = mod->y;
		es.sprite->fr = mod->sprite->states[mod->state_id].frames[es.frame];
	}
}


// losing a life.
void game_loose()
{ 
  // alternate 2 frames boss three times
  // then proceed to play if OK or send to game over
}


void game_instr_wait()
{
	if (PRESSED(select)) { // wait newly pressed
		frame_cb=game_instr_up;
	}
}

void game_instr_down()
{
	if (o_info->y<0) {
		o_info->y += 6;
	} else {
		frame_cb = game_instr_wait;
	}
}

void game_instr_up()
{
	if (o_info->y>-300) {
		o_info->y -= 6;
	} else {
		blitter_remove(o_info); o_info=0; // unload info
		frame_cb = game_intro;
	}
}


void game_intro() {
	if (PRESSED(select)) {
		message("instrs\n");	
		// SFX ?
		o_info=sprite_new(data_game_info_spr,0,-300,5); 
		frame_cb = game_instr_down;
	} else if PRESSED(start) {
		o_bg = sprite_new(data_game_background_spr,0,300,5); 
		frame_cb=game_level_pre;
	}

	// wait a bit
	// scroll bg from top
	// to menu
}


// find a sprite_id from sprite pointer
int sprite_id( const struct SpriteDef * spr ) 
{
	static const struct SpriteDef * sprtab[] = {
		&sprite_truck,
		&sprite_luigi,
		&sprite_mario,
		&sprite_status,
		&sprite_parcel,
	};
	for (int i=0;i<NBSPRITES;i++) {
		if (spr==sprtab[i]) return i;
	}
	die (9,9);
	return 0; // never reached
}

void game_init()
{
	// load data
	// init states

	frame_cb = game_intro;
	o_intro = sprite_new(data_game_intro_spr,0,0,10); // a bit behind others
	o_bottom = rect_new(0,250,400,50,20, RGB(255,255,255)); // white bottom

	// init sprite[state]->objectdef map
	for (int i=0;i<map_game.nb_objects;i++) {
		const int sprid=sprite_id(map_game.objects[i].sprite);
		map_objects[sprid][map_game.objects[i].state_id]=&map_game.objects[i];
	}

}

void game_level_pre()
{	
	if (o_bg->y>0) {
		o_bg->y-=6;
	} else { 
		o_intro->y = 1000; // hide
		o_bg->y=0;

		// start game
		score=0; 

		nbparcels = 0; // current nb

		// load all sprites & set to initial position
		truck.sprite=sprite_new(&data_truck_spr,0,0,0);
		truck.sprite_id = sprite_id(&sprite_truck);
		truck.state=state_truck_empty;
		truck.frame=0;

		mario.sprite=sprite_new(&data_mario_spr,0,0,0);
		mario.sprite_id = sprite_id(&sprite_mario);
		mario.state=state_mario_rest;
		mario.frame=0;

		luigi.sprite=sprite_new(&data_luigi_spr,0,0,0);
		luigi.sprite_id = sprite_id(&sprite_luigi);
		luigi.state=state_luigi_rest;
		luigi.frame=0;

		miss.sprite=sprite_new(&data_status_spr,0,0,0);
		miss.sprite_id = sprite_id(&sprite_status);
		miss.state=state_status_miss;
		miss.frame=0;

		for (int i=0;i<NBPARCELS;i++) {
			parcels[i].sprite=sprite_new(&data_parcel_spr,0,0,0);
			parcels[i].sprite_id = sprite_id(&sprite_parcel);
			parcels[i].state=255; // off 
			parcels[i].frame=0;
		}

		// start next 
		vga_frame=0;
		frame_cb=game_rest;
	}
}


// rest animation
void game_rest()
{
	mario.frame=luigi.frame=vga_frame/64;
	
	if (mario.frame==7) {
		// end of animation, start game
		enter_level();
		// show truck here ? 
	}
}

// loose animation
void frame_loose()
{
	struct ExtSprite *loser = mario_lost ? &mario : &luigi;

	loser->frame=vga_frame/32;
	const struct MapObjectDef *mod = map_objects[loser->sprite_id][loser->state];
	if (loser->frame == mod->sprite->states[mod->state_id].nb_frames) {
		// end of animation, start game
		// end or restart game
		if (miss.frame<3) {
			// re set loser in place
			loser->state=mario_lost ? state_mario_mid : state_luigi_mid;
			loser->frame=0;
			miss.frame++; 

			enter_level();
		} else {
			game_over();
		}
	}
}

void enter_loose()
{
	if (mario_lost) {
		mario.state=state_mario_miss;
		mario.frame=0;
	} else {
		luigi.state=state_luigi_miss;
		luigi.frame=0;
	}

	vga_frame=0;
	frame_cb = frame_loose;

}


void move_players( void )
{
	// check button presses (config ?)

	// Mario 
	if (PRESSED(up)) {
		switch (mario.state) {
			case state_mario_low: 
				mario.state=state_mario_mid;
				break;
			case state_mario_mid: 
				mario.state=state_mario_up;
				break;
		}
	}
	if (PRESSED(down)) {
		switch (mario.state) {
			case state_mario_mid: 
				mario.state=state_mario_low;
				break;
			case state_mario_up: 
				mario.state=state_mario_mid;
				break;
		}
	}

	// Luigi 
	if (PRESSED(A)) {
		switch (luigi.state) {
			case state_luigi_low: 
				luigi.state=state_luigi_mid;
				break;
			case state_luigi_mid: 
				luigi.state=state_luigi_up;
				break;
		}
	}
	if (PRESSED(B)) {
		switch (luigi.state) {
			case state_luigi_mid: 
				luigi.state=state_luigi_low;
				break;
			case state_luigi_up: 
				luigi.state=state_luigi_mid;
				break;
		}
	}

	// now check if needs moving parcels
	for (int i=0;i<NBPARCELS;i++) {
		if (parcels[i].waitframe>PARCEL_MOVESPEED) 
			continue; // too early to move yet

		if (parcels[i].state==state_parcel_zero && parcels[i].frame==2 && luigi.state==state_luigi_low) {
			parcels[i].state=state_parcel_one;
			luigi.frame=1;
			luigi.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==state_parcel_one && parcels[i].frame==7 && mario.state==state_mario_low) {
			parcels[i].state=state_parcel_two;
			mario.frame=1;
			mario.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==state_parcel_two && parcels[i].frame==7 && luigi.state==state_luigi_mid) {
			parcels[i].state=state_parcel_three;
			luigi.frame=1;
			luigi.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==state_parcel_three && parcels[i].frame==7 && mario.state==state_mario_mid) {
			parcels[i].state=state_parcel_four;
			mario.frame=1;
			mario.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==state_parcel_four && parcels[i].frame==7 && luigi.state==state_luigi_up) {
			parcels[i].state=state_parcel_five;
			luigi.frame=1;
			luigi.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==state_parcel_five && parcels[i].frame==7 && mario.state==state_mario_up) {
			// special : truck & remove 
			parcels[i].state=255;
			nbparcels-=1;
			
			truck_launch();

			mario.frame=1;
			mario.waitframe=10;
		} 
	}

	// reset frames if ongoing
	if (mario.waitframe)
		mario.waitframe--; 
	else 
		mario.frame=0;

	if (luigi.waitframe)
		luigi.waitframe--; 
	else 
		luigi.frame=0;

}

void new_parcel();
void prepare_parcel()
{
	static int prepare_time=0;
	if (!prepare_time) // none launched yet
		prepare_time=PARCEL_MOVESPEED*3;
	else {
		prepare_time--;		
		if (!prepare_time)
			new_parcel();
	}
}

void new_parcel()
{
	int pos=0;
	while ( parcels[pos].state != 255 && pos<NBPARCELS ) pos++;
	if (pos<NBPARCELS) {
		nbparcels++;
		parcels[pos].state = state_parcel_zero;
		parcels[pos].frame = 0;
		parcels[pos].waitframe=PARCEL_MOVESPEED;
	}
}

void move_parcel(struct ExtSprite *p)
{
	if (p->waitframe-- > 0) 
		return;

	int nb_steps = p->state==state_parcel_zero ? 3 : 8; 

	if (p->frame==nb_steps) {
		// breaking : lose a life				
		mario_lost = p->state == state_parcel_one || p->state == state_parcel_three || p->state == state_parcel_five;
		p->state=255;	
		nbparcels--;	
		enter_loose();
	} else {
		p->frame++;
		p->waitframe=p->frame==nb_steps-1 ? PARCEL_MOVESPEED : PARCEL_MOVESPEED*2;
	}

	// default : do nothing
}

void truck_launch()
{
	switch(truck.state) {
		case state_truck_empty : truck.state=state_truck_one;   break;
		case state_truck_one :   truck.state=state_truck_two;   break;
		case state_truck_two :   truck.state=state_truck_three; break;
		case state_truck_three : truck.state=state_truck_four;  break;
		case state_truck_four :  truck.state=state_truck_five;  break;
		case state_truck_five :  truck.state=state_truck_six;   break;
		case state_truck_six :   truck.state=state_truck_seven; break;
		case state_truck_seven : truck.state=state_truck_eight; break;
	}
	truck.frame=0;
	truck.waitframe = TRUCK_MOVESPEED;
}

void truck_move()
{
	if (truck.waitframe) {
		truck.waitframe--;
		if (truck.waitframe==0) {
			const struct MapObjectDef *mod = map_objects[truck.sprite_id][truck.state];
			if (truck.frame < mod->sprite->states[truck.state].nb_frames) {
				truck.frame+=1;
				truck.waitframe=TRUCK_MOVESPEED;
			}
		}
	}
}


// within game 
void frame_level()
{
	move_players();

	if (nbparcels<2) 
		prepare_parcel();

	for (int i=0;i<NBPARCELS;i++) {
		if (parcels[i].state!=255)
			move_parcel(&parcels[i]);
	}
	truck_move();
}

void enter_level()
{
	mario.state=state_mario_low;
	luigi.state=state_luigi_low;
	mario.frame=luigi.frame=0;
	frame_cb=frame_level;
}

void game_over()
{
	blitter_remove(o_bg); o_bg=0; // unload background & all other sprites
	for (int i=0;i<NBPARCELS;i++) {
		blitter_remove(parcels[i].sprite);
		parcels[i].sprite = 0;
	}
	blitter_remove(mario.sprite); mario.sprite=0;
	blitter_remove(luigi.sprite); luigi.sprite=0;
	blitter_remove(truck.sprite); truck.sprite=0;
	blitter_remove( miss.sprite);  miss.sprite=0;

	// show intro again
	o_intro->y = 0;
	frame_cb=game_intro;
}

void game_frame() 
{
	frame_cb();
	last_buttons_state=gamepad_buttons[0];

	// transfer ex_sprites to sprite frame, pos
	if (mario.sprite) { // all exist or none
		sprite_transfer(mario);
		sprite_transfer(luigi);
		sprite_transfer(truck);
		sprite_transfer(miss);
		for (int i=0;i<NBPARCELS;i++)
			sprite_transfer(parcels[i]);
	}
}