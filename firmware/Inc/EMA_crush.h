#ifndef EMA_cushhh
#define EMA_crushh

// provide a exponential moving average macro that should be unitless

//simple moving average
#define EMA_crush(in,ema,rate)  (ema = (in+((rate-1)*ema))/rate)

#endif
