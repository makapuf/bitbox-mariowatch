/* Mario Watch : a remake of 
   See original gameplay here : https://www.youtube.com/watch?v=03udgQDpQSA
*/
#include <stdint.h>
#include <stdbool.h>

#define NBPARCELS 6
#define PARCEL_MOVESPEED 30
#define TRUCK_MOVESPEED 20

#include "bitbox.h"
#include "lib/blitter/blitter.h"

#define DATA_IMPLEMENTATION
#include "data.h"

#define TILEMAPS_IMPLEMENTATION
#include "game.h"

struct ExtSprite {
	object *sprite;
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

int pos_state[game_st_nb][2]; // one position per state in this game


// ----

void sprite_transfer(struct ExtSprite es)
{
	// state 255 means hidden
	if (es.state==255) {
		es.sprite->y=9999;
	} else {
		es.sprite->fr = game_anims[es.state][es.frame];
		es.sprite->x = pos_state[es.state][0];
		es.sprite->y = pos_state[es.state][1];
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
		o_info=sprite_new(data_bin+data_offsets[data_game_info_spr],0,-300,5); 
		frame_cb = game_instr_down;
	} else if PRESSED(start) {
		o_bg = sprite_new(data_bin+data_offsets[data_game_background_spr],0,300,5); 
		frame_cb=game_level_pre;
	}

	// wait a bit
	// scroll bg from top
	// to menu
}


void game_init()
{
	// load data
	// init states

	frame_cb = game_intro;
	o_intro = sprite_new(data_bin+data_offsets[data_game_intro_spr],0,0,10); // a bit behind others
	o_bottom = rect_new(0,250,400,50,20, RGB(255,255,255)); // white bottom

	// link positions to state
	for (int i=0;i<game_objects_nb;i++) {
		pos_state[game_objects[i].state_id][0]=game_objects[i].x;
		pos_state[game_objects[i].state_id][1]=game_objects[i].y;
	}

}

void game_level_pre()
{	
	if (o_bg->y>0) {
		o_bg->y-=6;
	} else { 
		o_bg->y=0;

		// start game
		score=0; 

		nbparcels = 0; // current nb

		// load all sprites & set to initial position
		const char *spr=data_bin+data_offsets[data_game_truck_spr];
		truck.sprite=sprite_new(spr,0,0,0);
		truck.state=game_st_truck_empty;
		truck.frame=0;

		spr=data_bin+data_offsets[data_game_mario_spr];
		mario.sprite=sprite_new(spr,0,0,0);
		mario.state=game_st_mario_rest;
		mario.frame=0;

		spr=data_bin+data_offsets[data_game_luigi_spr];
		luigi.sprite=sprite_new(spr,0,0,0);
		luigi.state=game_st_luigi_rest;
		luigi.frame=0;

		spr=data_bin+data_offsets[data_game_miss_spr];
		miss.sprite=sprite_new(spr,0,0,0);
		miss.state=game_st_miss_status;
		miss.frame=0;

		spr=data_bin+data_offsets[data_game_parcel_spr];
		for (int i=0;i<NBPARCELS;i++) {
			parcels[i].sprite=sprite_new(spr,0,0,0);
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
	
	if (game_anims[loser->state][loser->frame]==255) {
		// end of animation, start game
		// replace loser 
		loser->state=mario_lost ? game_st_mario_mid : game_st_luigi_mid;
		loser->frame=0;
		enter_level();
	}
}

void enter_loose()
{
	if (mario_lost) {
		mario.state=game_st_mario_miss;
		mario.frame=0;
	} else {
		luigi.state=game_st_luigi_miss;
		luigi.frame=0;
	}

	vga_frame=0;
	frame_cb = frame_loose;

	miss.frame++; 
}


void move_players( void )
{
	// check button presses (config ?)

	// Mario 
	if (PRESSED(up)) {
		switch (mario.state) {
			case game_st_mario_low: 
				mario.state=game_st_mario_mid;
				break;
			case game_st_mario_mid: 
				mario.state=game_st_mario_up;
				break;
		}
	}
	if (PRESSED(down)) {
		switch (mario.state) {
			case game_st_mario_mid: 
				mario.state=game_st_mario_low;
				break;
			case game_st_mario_up: 
				mario.state=game_st_mario_mid;
				break;
		}
	}

	// Luigi 
	if (PRESSED(A)) {
		switch (luigi.state) {
			case game_st_luigi_low: 
				luigi.state=game_st_luigi_mid;
				break;
			case game_st_luigi_mid: 
				luigi.state=game_st_luigi_up;
				break;
		}
	}
	if (PRESSED(B)) {
		switch (luigi.state) {
			case game_st_luigi_mid: 
				luigi.state=game_st_luigi_low;
				break;
			case game_st_luigi_up: 
				luigi.state=game_st_luigi_mid;
				break;
		}
	}

	// now check if needs moving parcels
	for (int i=0;i<NBPARCELS;i++) {
		if (parcels[i].waitframe>PARCEL_MOVESPEED) 
			continue; // too early to move yet

		if (parcels[i].state==game_st_parcel_zero && parcels[i].frame==2 && mario.state==game_st_mario_low) {
			parcels[i].state=game_st_parcel_one;
			mario.frame=1;
			mario.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==game_st_parcel_one && parcels[i].frame==7 && luigi.state==game_st_luigi_low) {
			parcels[i].state=game_st_parcel_two;
			luigi.frame=1;
			luigi.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==game_st_parcel_two && parcels[i].frame==7 && mario.state==game_st_mario_mid) {
			parcels[i].state=game_st_parcel_three;
			mario.frame=1;
			mario.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==game_st_parcel_three && parcels[i].frame==7 && luigi.state==game_st_luigi_mid) {
			parcels[i].state=game_st_parcel_four;
			luigi.frame=1;
			luigi.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==game_st_parcel_four && parcels[i].frame==7 && mario.state==game_st_mario_up) {
			parcels[i].state=game_st_parcel_five;
			mario.frame=1;
			mario.waitframe=10;
			parcels[i].frame=0;
			parcels[i].waitframe=PARCEL_MOVESPEED;
		} else if (parcels[i].state==game_st_parcel_five && parcels[i].frame==7 && luigi.state==game_st_luigi_up) {
			// special : truck & remove 
			parcels[i].state=255;
			nbparcels-=1;
			
			truck_launch();

			luigi.frame=1;
			luigi.waitframe=10;
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
		parcels[pos].state = game_st_parcel_zero;
		parcels[pos].frame = 0;
		parcels[pos].waitframe=PARCEL_MOVESPEED;
	}
}

void move_parcel(struct ExtSprite *p)
{
	if (p->waitframe-- > 0) 
		return;

	int nb_steps = p->state==game_st_parcel_zero ? 3 : 8; 

	if (p->frame==nb_steps) {
		// breaking : lose a life				
		mario_lost = p->state == game_st_parcel_zero || p->state == game_st_parcel_two || p->state == game_st_parcel_four;
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
		case game_st_truck_empty : truck.state=game_st_truck_one; break;
		case game_st_truck_one : truck.state=game_st_truck_two; break;
		case game_st_truck_two : truck.state=game_st_truck_three; break;
		case game_st_truck_three : truck.state=game_st_truck_four; break;
		case game_st_truck_four : truck.state=game_st_truck_five; break;
		case game_st_truck_five : truck.state=game_st_truck_six; break;
		case game_st_truck_six : truck.state=game_st_truck_seven; break;
		case game_st_truck_seven : truck.state=game_st_truck_eight; break;
	}
	truck.frame=0;
	truck.waitframe = TRUCK_MOVESPEED;
}

void truck_move()
{
	if (truck.waitframe) {
		truck.waitframe--;
		if (truck.waitframe==0) {
			if (game_anims[truck.state][truck.frame+1]!=255) {
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
	mario.state=game_st_mario_low;
	luigi.state=game_st_luigi_low;
	mario.frame=luigi.frame=0;
	frame_cb=frame_level;
}

void game_over()
{
	blitter_remove(o_bg); o_bg=0; // unload background
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