/* Mario Watch */
/* TODO : rewrite game with bitbox_main instead of callbacks
objects dont move  : create them once place and that's it. show/hide accordingly.
all parcels "move" together, use set_state() -> simply change their sprite->data
*/

#include <stdint.h>
#include <stdbool.h>

#define NBPARCELS 6
#define PARCEL_MOVESPEED 30
#define TRUCK_MOVESPEED 20

#include "bitbox.h"
#include "sdk/lib/blitter/blitter.h"

// Resources handling  ---------------------------------

#include "data.h"
#include "game.h"

// references an embedded file - ptr to file
#define STATE(typ,st) &_binary_data_##typ##_##st##_spr_start

// list of oid to pointer to data
void *state_ptr[] = {
#define X(typ, st) STATE(typ,st),
game_OBJECTS
#undef X
};

// buttons just pressed. updates its state
uint16_t last_buttons_state; // last state of gamepad
uint16_t pressed;


// --- global vars

void (*frame_cb)(); // next callback.

struct object intro, bg, bottom;
struct object parcels[NBPARCELS];
struct object mario, luigi, truck, status;

int score, nbparcels; // number active

int truck_waitframe, luigi_waitframe, mario_waitframe, parcels_waitframe=PARCEL_MOVESPEED;
bool mario_lost, has_missed; // tracks who lost

// ---
void update_pressed( void ) {
	pressed = gamepad_buttons[0]&~last_buttons_state;
	last_buttons_state = gamepad_buttons[0]; // update button state
}
#define PRESSED(x) (pressed & gamepad_##x) // beware update internal state

const struct TilemapFileObjects *map = (struct TilemapFileObjects*) &((struct TilemapFile *)&_binary_data_game_map_start)->data;

// set state : load sprite and find initial position from map
void set_state(object *o,const void *file)
{
	sprite3_load(o, file);
	o->fr = 0;


	// find state position : find oid from ptr, find x,y from oid
	int oid,i;
	for (oid=0;state_ptr[oid]!=file; oid++); // no extra check, will be there
	// find x,y from this oid
	for (i=0;map[i].oid != oid;i++);
	o->x = map[i].x;
	o->y = map[i].y;
}

bool has_state(object *o, const void *state) {return (o->a == (intptr_t)state);}

#define SET_STATE(sprite, state) set_state(&sprite, STATE(sprite, state))
#define HAS_STATE(sprite, state) has_state(&sprite, STATE(sprite, state))

// hiding sprites
void spr_hide(object *o) { o->y |=  1024; }
void spr_show(object *o) { o->y &= ~1024; }
bool spr_is_hidden(object *o) { return o->y & 1024; }

// rest animation
void rest_animation()
{
	// set animation
	SET_STATE(mario,rest);
	SET_STATE(luigi,rest);
	for (int i=0;i<7*64;i++) {
		mario.fr=luigi.fr=i/64;
		wait_vsync();
	}
}

void truck_launch()
{
	static const void * truck_states[] = {
		STATE(truck, empty),
		STATE(truck, one),
		STATE(truck, two),
		STATE(truck, three),
		STATE(truck, four),
		STATE(truck, five),
		STATE(truck, six),
		STATE(truck, seven),
		STATE(truck, eight),
	};

	for (int i=0;i<sizeof(truck_states)/sizeof(void*);i++) {
		if (has_state(&truck, truck_states[i])) {
			set_state(&truck, truck_states[i+1]);
			break;
		}
	}

	truck_waitframe = TRUCK_MOVESPEED;
}

void truck_move()
{
	if (truck_waitframe) {
		truck_waitframe--;
		if (truck_waitframe==0) {
			if (truck.fr < sprite3_nbframes(&truck)-1) {
				truck.fr++;
				truck_waitframe=TRUCK_MOVESPEED;
			}
		}
	}
}

void move_players( void )
{
	update_pressed();
	// Mario - left
	if (PRESSED(up)) {
		if      (HAS_STATE(mario, low)) SET_STATE(mario, mid);
		else if (HAS_STATE(mario, mid)) SET_STATE(mario, up);
	}
	if (PRESSED(down)) {
		if      (HAS_STATE(mario, mid)) SET_STATE(mario, low);
		else if (HAS_STATE(mario, up))  SET_STATE(mario, mid);
	}

	// Luigi
	if (PRESSED(A)) {
		if      (HAS_STATE(luigi, low)) SET_STATE(luigi, mid);
		else if (HAS_STATE(luigi, mid)) SET_STATE(luigi, up);
	}
	if (PRESSED(B)) {
		if      (HAS_STATE(luigi, mid)) SET_STATE(luigi, low);
		else if (HAS_STATE(luigi, up))  SET_STATE(luigi, mid);
	}

	// if automove, do it
	if (mario_waitframe)
		mario_waitframe--;
	else
		mario.fr=0;

	if (luigi_waitframe)
		luigi_waitframe--;
	else
		luigi.fr=0;

}

void new_parcel()
{
	int pos;
	for (pos=0; pos<NBPARCELS; pos++)
		if (spr_is_hidden(&parcels[pos])) break;

	if (pos<NBPARCELS) {
		nbparcels++;
		set_state(&parcels[pos], STATE(parcel, zero));
	}
}

// check if a new parcel is needed
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

// check if player can move parcels
void player_moveparcel(struct object *p)
{
	if (has_state(p, STATE(parcel,zero))  && p->fr==2 && HAS_STATE(luigi, low)) {
		set_state(p, STATE(parcel, one));
		luigi.fr=1;
		luigi_waitframe=10;
	} else if (has_state(p, STATE(parcel, one))  && p->fr==7 && HAS_STATE(mario, low)) {
		set_state(p, STATE(parcel, two));
		mario.fr=1;
		mario_waitframe=10;
	} else if (has_state(p, STATE(parcel, two))  && p->fr==7 && HAS_STATE(luigi, mid)) {
		set_state(p, STATE(parcel, three));
		luigi.fr=1;
		luigi_waitframe=10;
		p->fr=0;
	} else if (has_state(p, STATE(parcel, three))&& p->fr==7 && HAS_STATE(mario, mid)) {
		set_state(p, STATE(parcel, four));
		mario.fr=1;
		mario_waitframe=10;
		p->fr=0;
	} else if (has_state(p, STATE(parcel, four)) && p->fr==7 && HAS_STATE(luigi, up)) {
		set_state(p, STATE(parcel, five));
		luigi.fr=1;
		luigi_waitframe=10;
		p->fr=0;
	} else if (has_state(p, STATE(parcel, five)) && p->fr==7 && HAS_STATE(mario, up)) {
		// special : truck & remove
		spr_hide(p);
		nbparcels-=1;

		truck_launch();

		mario.fr=1;
		mario_waitframe=10;
	}
}

void move_parcel(struct object *p)
{
	if (p->fr==sprite3_nbframes(p)-1) {
		// breaking : lose a life
		mario_lost = has_state(p, STATE(parcel, one)) ||\
			has_state(p, STATE(parcel, three)) ||\
			has_state(p, STATE(parcel, five));
		spr_hide(p);
		nbparcels--;
		has_missed = true;
	} else {
		p->fr++;
		// wait twice at end of anim -> set two frames ?
		// p->waitframe=p->frame==nb_steps-1 ? PARCEL_MOVESPEED : PARCEL_MOVESPEED*2;
	}
	player_moveparcel(p);
}

// ----------------------------------------------------------------------------------------------------
// - game status

void life_start()
{
	has_missed = false;
}

void life_play()
{
	while (!has_missed)
	{
		move_players();
		if (nbparcels<2)
			prepare_parcel();

		if (parcels_waitframe == 0) {
			for (int i=0;i<NBPARCELS;i++) {
				if (!spr_is_hidden(&parcels[i]))
					move_parcel(&parcels[i]);
			}
			parcels_waitframe = PARCEL_MOVESPEED;
		}
		parcels_waitframe--;

		truck_move();
		wait_vsync();
	}
}

void life_miss()
{
	struct object *loser = mario_lost ? &mario : &luigi;
	if (mario_lost) {
		SET_STATE(mario, miss);
	} else {
		SET_STATE(luigi, miss);
	}

	for (int i=0;i<sprite3_nbframes(loser)*32;i++) {
		loser->fr=i/32;
		wait_vsync();
	}

	if (mario_lost) {
		SET_STATE(mario, mid);
	} else {
		SET_STATE(luigi, mid);
	}

	status.fr++;
}


void game_start()
{
	score=0;
	nbparcels = 0; // current nb

	// load bg and show it
	sprite3_load(&bg,&_binary_data_background_spr_start);
	blitter_insert(&bg,0,-300,5);
	while (bg.y<0) {
		bg.y += 6;
		wait_vsync();
	}
	bg.y=0;
	spr_hide(&intro);

	// reset sprites state
	blitter_insert(&truck,0,0,0); // here we insert before setting state since it also put in place
	SET_STATE(truck,empty);

	blitter_insert(&status,0,0,0);
	SET_STATE(status,miss);

	for (int i=0;i<NBPARCELS;i++) {
	 	blitter_insert(&parcels[i],0,0,0);
		set_state(&parcels[i],STATE(parcel,zero)); // any
		spr_hide(&parcels[i]);
	}

	blitter_insert(&mario,0,0,0);
	blitter_insert(&luigi,0,0,0);

	rest_animation();

	SET_STATE(mario,low);
	SET_STATE(luigi,low);
}

void game_over()
{

	blitter_remove(&mario);
	blitter_remove(&luigi);
	blitter_remove(&truck);
	blitter_remove(&status);

	for (int i=0;i<NBPARCELS;i++)
		blitter_remove(&parcels[i]);

	blitter_remove(&bg); // unload background & all other sprites
}

void show_instructions()
{
	struct object info;
	sprite3_load(&info, &_binary_data_info_spr_start);
	blitter_insert(&info,0,-300,5);

	while (info.y<0)         { info.y += 6; wait_vsync(); update_pressed(); }      // down
	while (!PRESSED(select)) { wait_vsync(); update_pressed(); } // wait
	while (info.y>-300)      { info.y -= 6; wait_vsync(); update_pressed(); }      // up

	blitter_remove(&info);
}

void game_intro()
{
	// show intro - hide bg
	while(!PRESSED(start)) {
		if (PRESSED(select)) {
			// SFX ?
			show_instructions();
		}
		wait_vsync(); update_pressed();
	}
}

void bitbox_main()
{
	// never released, always present.
	static object bottom;
	rect_init(&bottom, 400,50, RGB(255,255,255)); // white bottom : needed ?
	blitter_insert(&bottom, 0,250,20);

	// load intro screen
	sprite3_load  (&intro, &_binary_data_intro_spr_start);
	blitter_insert(&intro,0,0,10);

	while (1) {
		game_intro();
		game_start();
		message("start\n");
		while (status.fr < 3) {
			life_start();
			life_play();
			life_miss();
		}
		game_over();
	}
}
