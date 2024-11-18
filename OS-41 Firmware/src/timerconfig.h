//Timer configurations for ISR

#define TIMER_INTERRUPT_DEBUG         0 //Timer debug level (necessary for ESP8266TimerInterrupt)
#define _TIMERINTERRUPT_LOGLEVEL_     0
// Select a Timer Clock
#define USING_TIM_DIV1                false           // for shortest and most accurate timer
#define USING_TIM_DIV16               false           // for medium time and medium accurate timer
#define USING_TIM_DIV256              true            // for longest timer but least accurate. Default

#define TIMER_INTERVAL_MS        100 //Interval in ms