#include "Watchdog.h"

uint8_t isSet = 0;

Watchdog::Watchdog(void) {
}

uint8_t Watchdog::enable( uint32_t timeInterval ) {
	if( isSet ) {
		return 1;
	}

	if( (timeInterval < 1) || (timeInterval > 16) ) {
		timeInterval = 16;
	}

	timeInterval = timeInterval * 256;
	WDT_Enable( WDT , 0x2000 | timeInterval | (timeInterval << 16 ) );

	isSet = 1;
	return 0;
}

void Watchdog::restart() {
	WDT_Restart( WDT );
}


//disable the WDT
uint8_t Watchdog::disable() {
	if( isSet ) {
		return 1;
	}
	WDT_Disable( WDT );
	return 0;
}

Watchdog watchdog;