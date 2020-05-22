#ifndef statemachineh
#define statemachineh
#define uint32 uint32_t 

enum fsmstate { 
	fsm_init,
	fsm_nominal,
	fsm_notify,
	fsm_alarm,
	fsm_alarmmute,
	fsm_exception,
	fsm_last
};

enum fsmstate gfsmstate=fsm_init;

typedef enum  condition_id {
	button_mute,
	parameter_violation,
	sensor_error,
	condidion_last
} condition_id_t;


typedef struct condidtionstate {
	condition_id_t condition_id;
	uint32 	start_time; // in ms
	uint32 	stop_time;
} conditionstate_t;

void 	condition_start (conditionstate_t * );
void 	condition_stop (conditionstate_t *);
// duration in milliseconnds
uint32 	condition_duration (conditionstate_t * ); 
// convenience call for duration in seconds
uint32 	condition_duration_s (conditionstate_t * ); 

conditionstate_t cvec[condidion_last];
/* allocate condition global this permits use like:
  if (condidion_duration_s(&cvec[sensor_error]) > 20 ) ?  do_senserrorthing() : 0
*/
#endif
