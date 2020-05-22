#ifndef keyscanh
#define keyscanh 

#define keyscan_debounce_ms 23

enum keyscanstate {
        key_init,
        key_down,
        key_up,
        key_last
};

typedef struct keyscan_s {
	uint32 keychangetime;
	void (*keyuproutine)();
	void (*keydownroutine)();
	enum keyscanstate keystate;
	enum keyscanstate lastkeystate;
	int  keydelta;
	int  holddown; 
} keyscan_t;

void keyscan( uint32 ticks, keyscan_t *k, int in_val) {
  /* in-val is either 0 ( up ) or nonzero (down)
		ticks should be provided from HAL_GetTick() closely befor calling keyscan
	  provide debounced state in keystate, 
	  run routines ( once!) on up and down events */

	if ( in_val &&  // button down
		in_val == k->keydelta  && // button state is still down
		ticks - k->keychangetime > keyscan_debounce_ms &&  // button has been down for at least X ms
		k->holddown == 0 ) // button event has allready been dispatched
	{ 
		k->keystate = key_down; // memoize the keystate
		k->holddown = 1; 
		k->lastkeystate = k->keystate; // remember so we dont double dispatch
		if ( k->keydownroutine) (*(k->keydownroutine)) (); // run the callback
	} else if ( in_val == k->keydelta &&  // button state is still up
		ticks - k->keychangetime > keyscan_debounce_ms &&  // debounce the button up
		k->holddown ==0 	) // don't run if already dispatched
	//button Up candidate 
	{ 
		k->keystate = key_up;
		k->holddown = 1; 
		k->lastkeystate = k->keystate;
		if ( k->keyuproutine) (*(k->keyuproutine)) ();
	}
	// keep a journal of the last time key was changed
	if (in_val != k->keydelta) {
			k->keychangetime=ticks; 
			k->holddown =0; 
	}
	k->keydelta = in_val;
}
void keyscan_init( keyscan_t * k ) {
	k->keychangetime = 0;
	k->keydelta = 0;
	k->holddown = 0;
	k->keydownroutine = k->keyuproutine = 0;
	k->keystate = key_up; // assume button is not mashed at startp
	k->lastkeystate = key_down;
}
	
uint32 keyscan_duration(uint32 ticks, keyscan_t *k) {
/* report how many ms the key has been in it's current confiuration */
	return (ticks - k->keychangetime); 
}

#endif
