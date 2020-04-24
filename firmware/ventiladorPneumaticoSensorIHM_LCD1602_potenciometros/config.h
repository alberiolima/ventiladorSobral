#ifndef _CONFIG_H_
#define _CONFIG_H_

#define LCD1602_I2C
//#define SENSOR_BMP180
//#define SENSOR_BMP280

#define hPa_mmH2O (10.197162129779283)
#define Pa_mmH2O (0.10197162129779)

#define ANALOG_MIN 0
#define ANALOG_MAX 1023

#define POWER_ON LOW
#define TEMPO_STATUS 3000

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO) //Arduino Uno e Nano
  #define PORTA_POT1 A0
  #define PORTA_POT2 A1
  #define PORTA_POT3 A2
  #define PORTA_POT4 A3
  #define PORTA_POT5 A6
  #define PORTA_POT6 A7
  #define PINO_POWER  7
  #define PINO_STATUS 8
  #define PINO_VALVULA_EXPIRACAO   9
  #define PINO_VALVULA_INSPIRACAO 10  
#elif defined(ARDUINO_AVR_LEONARDO) || defined(ARDUINO_AVR_MICRO) //Arduino Leonardo ou Arduino ProMicro
  #define PORTA_POT1 A0
  #define PORTA_POT2 A1
  #define PORTA_POT3 A2
  #define PORTA_POT4 A3
  #define PORTA_POT5 A6
  #define PORTA_POT6 A7
  #define PINO_POWER  7
  #define PINO_STATUS 8
  #define PINO_VALVULA_EXPIRACAO   9
  #define PINO_VALVULA_INSPIRACAO 10  
#elif defined(ARDUINO_GENERIC_STM32F103C) //bluepill stm32  
  #define PORTA_POT1 PA0
  #define PORTA_POT2 PA1
  #define PORTA_POT3 PA2
  #define PORTA_POT4 PA3
  #define PORTA_POT5 PA4
  #define PORTA_POT6 PA5
  #define PINO_VALVULA_EXPIRACAO   9
  #define PINO_VALVULA_INSPIRACAO 10    
#else
  #error Plataforma nao definida  
#endif

#ifdef PINO_VALVULA_INSPIRACAO
  #define abreValvulaInspiracao()  digitalWrite( PINO_VALVULA_INSPIRACAO, LOW )  // Abre válvula da inspiração
  #define fechaValvulaInspiracao() digitalWrite( PINO_VALVULA_INSPIRACAO, HIGH ) // Fecha válvula da inspiração
#endif
#ifdef PINO_VALVULA_EXPIRACAO
  #define abreValvulaExpiracao()  digitalWrite( PINO_VALVULA_EXPIRACAO, LOW )  // Abre válvula da expiração
  #define fechaValvulaExpiracao() digitalWrite( PINO_VALVULA_EXPIRACAO, HIGH ) // Fecha válvula da expiração
  #define CONTROLE_PEEP //Só faz controle de PEEP se a valvula não for automática, pois a valvula automatica já deve fazer isso
#endif  

#endif
