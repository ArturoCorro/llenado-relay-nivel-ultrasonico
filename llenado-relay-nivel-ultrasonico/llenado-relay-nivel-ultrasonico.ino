/*
	Dosificador sin pantall

	Por Arturo Corro
	8 de Enero del 2021
*/

#include <EEPROM.h>


/**************************************************
* Definición
**************************************************/
	/* Definición de Hardware - PINOUT
	**************************************************/
		#define SEGMENT_A 12
		#define SEGMENT_B A5
		#define SEGMENT_C 3
		#define SEGMENT_D 2
		#define SEGMENT_E 4
		#define SEGMENT_F A3
		#define SEGMENT_G A2

		#define BUTTON_ONE_SETTINGS_PIN A1
		#define BUTTON_TWO_SETTINGS_PIN A0

		#define SENSOR_MAX_LEVEL_PIN 6
		#define SENSOR_MIN_LEVEL_PIN 5

		#define SSR_PIN 7


	/* Definición de Hardware - EEPROM ADDRESS
	**************************************************/
		#define BUTTON_ONE_SETTINGS_EEPROM_ADDRESS 0
		#define BUTTON_TWO_SETTINGS_EEPROM_ADDRESS 1


	/* Definición de Setup
	**************************************************/
		#define BUTTON_NOT_PRESSED 1
		#define BUTTON_PRESSED     0

		#define BUTTON_STATE_IN_IDLE         0
		#define BUTTON_STATE_EXECUTE_COMMAND 1

		#define PRESSED_BUTTON_DELAY_MILLIS        200
		#define AUTOMATIC_CLOSING_SETTINGS_MILLIS 5000

		#define SETUP_POSITION_OUT        0
		#define SETUP_POSITION_BUTTON_ONE 1
		#define SETUP_POSITION_BUTTON_TWO 2

		#define BUTTON_ONE_MIN_LEVEL 0
		#define BUTTON_ONE_MAX_LEVEL 18
		#define BUTTON_TWO_MIN_LEVEL 0
		#define BUTTON_TWO_MAX_LEVEL 18


	/* Definición tank filling system
	**************************************************/
		#define DETECTING_LIQUID     0
		#define NOT_DETECTING_LIQUID 1

		#define SSR_SHUTDOWN_BY_SENSOR_LEVEL_HIGH 19
		#define SSR_SWITCHED_ON           		  22
		#define SSR_SHUTDOWN_BY_EXCEEDED_TIME     20
		#define SSR_SHUTDOWN_BY_PUMP_PROTECTION   21

		#define SSR_ON  1
		#define SSR_OFF 0


	/* Debug
	**************************************************/
		/**
		 * Define DEBUG_SERIAL_ENABLE to enable debug serial.
		 * Comment it to disable debug serial.
		 */
		//#define DEBUG_SERIAL_ENABLE
		
		/**
		 * Define dbSerial for the output of debug messages.
		 */
		#define dbSerial Serial

		#ifdef DEBUG_SERIAL_ENABLE
			#define serialPrint(a)    dbSerial.print(a)
			#define serialPrintln(a)  dbSerial.println(a)
			#define serialBegin(a)    dbSerial.begin(a)
			
			String debug_ssr_state[] = {"SENS OFF", "SSR  ON ", "TIME OFF", "PROTECT "};
			uint32_t _timing_debug;

		#else
			#define serialPrint(a)    do{}while(0)
			#define serialPrintln(a)  do{}while(0)
			#define serialBegin(a)    do{}while(0)
		#endif


/**************************************************
* Variables de la aplicación
**************************************************/
	/* Configuraciones
	**************************************************/
		uint8_t button_status;
		uint8_t settings_within;

		uint8_t one_setting_value;
		uint8_t two_setting_value;

		uint32_t button_pressed_millis;
		uint32_t button_pressed_last_time_millis;

		uint32_t _timing;


	/* Sensors and SSR status
	**************************************************/
		uint8_t  max_time_on_levels[]     = {0, 2, 3, 4, 5, 6, 8, 10, 13, 16, 20, 24, 29, 35, 42, 50, 59, 70, 90};
		uint16_t pump_protection_levels[] = {0, 10, 20, 30, 40, 60, 90, 120, 180, 240, 300, 390, 480, 600, 720, 840, 1020, 1200, 1440};

		uint32_t last_ignition_millis;
		uint32_t last_shutdown_millis;

		uint8_t ssr_status;


/**************************************************
* Métdodos de la aplicación
**************************************************/
	/* initializations
	**************************************************/
		void deviceInit()
		{
			Serial.begin(9600);

			serialPrintln("inicializo el Dosificador");

			HardwInit();

			// Cargamos configuraciones de EEPROM
			one_setting_value = getOneSettingValue();
			two_setting_value = getTwoSettingValue();

			debugSettings();
			turnOffAllSegments();
		}

		void HardwInit()
		{
			pinMode(BUTTON_ONE_SETTINGS_PIN, INPUT_PULLUP);
			pinMode(BUTTON_TWO_SETTINGS_PIN, INPUT_PULLUP);

			pinMode(SENSOR_MIN_LEVEL_PIN, INPUT_PULLUP);
			pinMode(SENSOR_MAX_LEVEL_PIN, INPUT_PULLUP);

			pinMode(SSR_PIN, OUTPUT);

			pinMode(SEGMENT_A, OUTPUT);
			pinMode(SEGMENT_B, OUTPUT);
			pinMode(SEGMENT_C, OUTPUT);
			pinMode(SEGMENT_D, OUTPUT);
			pinMode(SEGMENT_E, OUTPUT);
			pinMode(SEGMENT_F, OUTPUT);
			pinMode(SEGMENT_G, OUTPUT);
		}


	/* Helpers Configuraciones
	**************************************************/
		void enterASettings(uint8_t param_setting)
		{
			settings_within = param_setting;

			switch(settings_within){
				case SETUP_POSITION_BUTTON_ONE:
					displayDigit(one_setting_value);
					break;

				case SETUP_POSITION_BUTTON_TWO:
					displayDigit(two_setting_value);
					break;
			}

			serialPrintln("Entro a configuraciones");
			debugSettings();
		}

		void automaticClosedSettings()
		{
			if(settings_within != SETUP_POSITION_OUT && button_pressed_last_time_millis + AUTOMATIC_CLOSING_SETTINGS_MILLIS <= millis()){
				settings_within = SETUP_POSITION_OUT;
				button_pressed_last_time_millis = 0;

				turnOffAllSegments();

				serialPrintln("Salio de configuraciones");
				debugSettings();
			}
		}


	/* Display 7 segmentos
	**************************************************/
		void displayDigit(uint8_t digit)
		{
			turnOffAllSegments();

			//Conditions for displaying segment a
			if(digit != 1 && digit != 4 && digit != 14 && digit != 15 && digit != 16 && digit != 18 && digit != 19)
				digitalWrite(SEGMENT_A, HIGH);

			//Conditions for displaying segment b
			if(digit != 5 && digit != 6 && digit != 11 && digit != 12 && digit != 13 && digit != 16 && digit != 19 && digit != 20 && digit != 21)
				digitalWrite(SEGMENT_B, HIGH);

			//Conditions for displaying segment c
			if(digit != 2 && digit != 11 && digit != 12 && digit != 13 && digit != 16 && digit != 17 && digit != 19 && digit != 20 && digit != 21 && digit != 22)
				digitalWrite(SEGMENT_C, HIGH);

			//Conditions for displaying segment d
			if(digit != 1 && digit != 4 && digit != 7 && digit != 9 && digit != 10 && digit != 13 && digit != 14 && digit != 17 && digit != 19 && digit != 20 && digit != 22)
				digitalWrite(SEGMENT_D, HIGH);

			//Conditions for displaying segment e
			if (digit != 1 && digit != 3 && digit != 4 && digit != 5 && digit != 7 && digit != 9 && digit != 15 && digit != 19 && digit != 20 && digit != 21 && digit != 22)
				digitalWrite(SEGMENT_E, HIGH);

			//Conditions for displaying segment f
			if(digit != 1 && digit != 2 && digit != 3 && digit != 7 && digit != 15 && digit != 19 && digit != 20 && digit != 21)
				digitalWrite(SEGMENT_F, HIGH);

			if (digit != 0 && digit != 1 && digit != 7 && digit != 11 && digit != 15 && digit != 16 && digit != 18 && digit != 19)
				digitalWrite(SEGMENT_G, HIGH);
		}

		void turnOffAllSegments()
		{
			digitalWrite(SEGMENT_A, LOW);
			digitalWrite(SEGMENT_B, LOW);
			digitalWrite(SEGMENT_C, LOW);
			digitalWrite(SEGMENT_D, LOW);
			digitalWrite(SEGMENT_E, LOW);
			digitalWrite(SEGMENT_F, LOW);
			digitalWrite(SEGMENT_G, LOW);
		}

		void turnOnAllSegments()
		{
			digitalWrite(SEGMENT_A, HIGH);
			digitalWrite(SEGMENT_B, HIGH);
			digitalWrite(SEGMENT_C, HIGH);
			digitalWrite(SEGMENT_D, HIGH);
			digitalWrite(SEGMENT_E, HIGH);
			digitalWrite(SEGMENT_F, HIGH);
			digitalWrite(SEGMENT_G, HIGH);
		}


	/* update EEPROM settings
	**************************************************/
		void nextConfigurationValue(uint8_t button_settings)
		{
			switch(button_settings){
				case SETUP_POSITION_BUTTON_ONE:
					one_setting_value = (one_setting_value >= BUTTON_ONE_MAX_LEVEL)?
						BUTTON_ONE_MIN_LEVEL:
						one_setting_value + 1;

					EEPROM.write(BUTTON_ONE_SETTINGS_EEPROM_ADDRESS, one_setting_value);

					displayDigit(one_setting_value);
					break;

				case SETUP_POSITION_BUTTON_TWO:
					two_setting_value = (two_setting_value >= BUTTON_TWO_MAX_LEVEL)?
						BUTTON_TWO_MIN_LEVEL:
						two_setting_value + 1;

					EEPROM.write(BUTTON_TWO_SETTINGS_EEPROM_ADDRESS, two_setting_value);

					displayDigit(two_setting_value);
					break;
			}

			serialPrintln("nextConfigurationValue()");
			debugSettings();
		}


	/* Get EEPROM Settings
	**************************************************/
		uint32_t getOneSettingValue()
		{
			return EEPROM.read(BUTTON_ONE_SETTINGS_EEPROM_ADDRESS);
		}

		uint32_t getTwoSettingValue()
		{
			return EEPROM.read(BUTTON_TWO_SETTINGS_EEPROM_ADDRESS);
		}


	/* Helpers filling process
	**************************************************/
		void ssrState(uint8_t _ssr_state){
			ssr_status = _ssr_state;

			switch(ssr_status){
				case SSR_SWITCHED_ON:
					if(last_ignition_millis == 0){
						last_ignition_millis = millis();
						last_shutdown_millis = 0;
					}
					digitalWrite(SSR_PIN, SSR_ON);
					break;

				case SSR_SHUTDOWN_BY_SENSOR_LEVEL_HIGH:
				case SSR_SHUTDOWN_BY_EXCEEDED_TIME:
				case SSR_SHUTDOWN_BY_PUMP_PROTECTION:
					if(last_shutdown_millis == 0){
						last_ignition_millis = 0;
						last_shutdown_millis = millis();
					}
					digitalWrite(SSR_PIN, SSR_OFF);
					break;
			}

			if(!settings_within)
				displayDigit(ssr_status);
		}

		uint8_t shutdownToMaximumTimeExceeded()
		{
			if(one_setting_value == 0 || last_ignition_millis == 0)
				return false;

			boolean maximum_time_exceeded = (last_ignition_millis + (max_time_on_levels[one_setting_value] * 60000)) <= millis();
			

			if(maximum_time_exceeded)
				ssrState(SSR_SHUTDOWN_BY_EXCEEDED_TIME);

			return maximum_time_exceeded;
		}

		uint8_t shutdownForPumpProtection()
		{
			if(two_setting_value == 0 || last_shutdown_millis == 0)
				return false;

			boolean protection_time2 = !((last_shutdown_millis + (pump_protection_levels[two_setting_value] * 60000)) <= millis());

			if(protection_time2)
				ssrState(SSR_SHUTDOWN_BY_PUMP_PROTECTION);

			return protection_time2;
		}


	/* Loops
	**************************************************/
		void setupLoop()
		{
			// ningun boton precionado
			if(digitalRead(BUTTON_ONE_SETTINGS_PIN) == BUTTON_PRESSED){
				if(button_pressed_millis == 0)
					button_pressed_millis = millis() + 100;

				if(button_status == BUTTON_STATE_IN_IDLE){
					if(button_pressed_millis <= millis()){
						if(settings_within == SETUP_POSITION_BUTTON_ONE)
							nextConfigurationValue(SETUP_POSITION_BUTTON_ONE);
						else
							enterASettings(SETUP_POSITION_BUTTON_ONE);

						button_status = BUTTON_STATE_EXECUTE_COMMAND;
						button_pressed_last_time_millis = millis();
					}
				}

			}else if(digitalRead(BUTTON_TWO_SETTINGS_PIN) == BUTTON_PRESSED){
				if(button_pressed_millis == 0)
					button_pressed_millis = millis() + 100;

				if(button_status == BUTTON_STATE_IN_IDLE){
					if(button_pressed_millis <= millis()){
						if(settings_within == SETUP_POSITION_BUTTON_TWO)
							nextConfigurationValue(SETUP_POSITION_BUTTON_TWO);

						else
							enterASettings(SETUP_POSITION_BUTTON_TWO);

						button_status = BUTTON_STATE_EXECUTE_COMMAND;
						button_pressed_last_time_millis = millis();
					}
				}

			}else{
				// Reseteo de estatus de botones
				button_status = BUTTON_STATE_IN_IDLE;
				button_pressed_millis = 0;
			}

			// Proceso para salir de configuraciones
			if(_timing <= millis()){
				_timing += 1000;

				automaticClosedSettings();
			}
		}

		void waterTankFillingLoop()
		{
			if(ssr_status == SSR_SWITCHED_ON){
				// Si sensor de Nivel Alto detecta liquido, se mantiene apagado
				if(digitalRead(SENSOR_MAX_LEVEL_PIN) == DETECTING_LIQUID){
					ssrState(SSR_SHUTDOWN_BY_SENSOR_LEVEL_HIGH);

				// Nivel Alto NO detecta liquido
				}else{
					if(shutdownToMaximumTimeExceeded())
						return;

					if(shutdownForPumpProtection())
						return;
				}

			// SSR_OFF
			}else{
				// Si sensor de nivel bajo sin liquido, se tienen que encender SSR
				if(digitalRead(SENSOR_MIN_LEVEL_PIN) == NOT_DETECTING_LIQUID){
					if(shutdownToMaximumTimeExceeded())
						return;

					if(shutdownForPumpProtection())
						return;

					ssrState(SSR_SWITCHED_ON);
				}
			}
		}


	/* Debug
	**************************************************/
		void debugSettings()
		{
			#ifdef DEBUG_SERIAL_ENABLE
				serialPrint("settings_within: ");
				serialPrint(settings_within);

				serialPrint("   one_setting_value: ");
				serialPrint(one_setting_value);

				serialPrint("   two_setting_value: ");
				serialPrint(two_setting_value);

				serialPrintln("");
			#endif
		}

		void debugFillingProcess()
		{
			#ifdef DEBUG_SERIAL_ENABLE
				if(_timing_debug <= millis()){
					_timing_debug += 1000;

					serialPrint("ignition: ");
					serialPrint(last_ignition_millis / 1000);

					serialPrint(",  shutdown: ");
					serialPrint(last_shutdown_millis / 1000);

					uint32_t max_time = last_ignition_millis?
						((last_ignition_millis + (max_time_on_levels[one_setting_value] * 60000)) - millis()) / 1000:
						0;
					serialPrint(",  MaxTime: ");
					serialPrint(max_time);

					uint32_t protection_time = last_shutdown_millis?
						(millis() >= last_shutdown_millis + (pump_protection_levels[two_setting_value] * 60000)?
							0:
							((last_shutdown_millis + (pump_protection_levels[two_setting_value] * 60000)) - millis()) / 1000):
						0;
					serialPrint(",  ProtectionTime: ");
					serialPrint(protection_time);

					serialPrint(",  SSR: ");

					uint8_t ssr_status_idx = 0;

					switch(ssr_status){
						case SSR_SHUTDOWN_BY_SENSOR_LEVEL_HIGH:
							ssr_status_idx = 0;
							break;

						case SSR_SWITCHED_ON:
							ssr_status_idx = 1;
							break;

						case SSR_SHUTDOWN_BY_EXCEEDED_TIME:
							ssr_status_idx = 2;
							break;

						case SSR_SHUTDOWN_BY_PUMP_PROTECTION:
							ssr_status_idx = 3;
							break;
					}

					serialPrint(debug_ssr_state[ssr_status_idx]);
					serialPrintln("");
				}
			#endif
		}


/**************************************************
* SETUP & LOOP
**************************************************/
	void setup()
	{
		deviceInit();
	}

	void loop()
	{
		setupLoop();
		waterTankFillingLoop();
		debugFillingProcess();
	}
