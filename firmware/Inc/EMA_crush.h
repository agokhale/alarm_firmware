#ifndef EMA_cushhh
#define EMA_crushh

/*  provide a exponential moving average macro that should be unitless
 simple moving average
vars: 
	in: most current value
	ema: last value, also overwritten in this operation
	rate: smoothing rate,  divisor for the delta to be applied to the old value; extra credit for useing powers of 2, which saves some cpu time.

usage:  EMA_crush(newval, stored_ema, 32);
*/

#define EMA_crush(in,ema,rate)  (  \
	ema = ( \
		 ema + (in - ema)/rate  \
		)  \
	)

#endif
