#ifndef WATCHDOG_H

	#define WATCHDOG_H

	#include <Arduino.h>

	class Watchdog {
		public:
			Watchdog( void );
			uint8_t enable( uint32_t timeInterval = 16 );
	        uint8_t disable( void );
	        void restart( void );

		private:
			//private methods
	};

	extern Watchdog watchdog;

#endif
