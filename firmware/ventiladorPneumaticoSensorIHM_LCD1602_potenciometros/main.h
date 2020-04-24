#ifdef SENSOR_BMP180
  #include <bmp180.h>
  bmp180 sensorPressaoBMP180;
#endif
#ifdef SENSOR_BMP280
  #include <BME280I2C.h>
  #include <Wire.h>
  BME280I2C::Settings settings(
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::Mode_Forced,
    BME280::StandbyTime_20ms,
    BME280::Filter_Off,
    BME280::SpiEnable_False,
    0x76 // I2C address. I2C specific.
  );

  BME280I2C sensorPressaoBMP280(settings);
#endif
#include <Button.h>
Button btStatus(PINO_STATUS);

#ifdef LCD1602_I2C
  /* Objeto do display 16x2*/
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);
#endif
#if defined(SENSOR_BMP280)|| defined(SENSOR_BMP180)
  /* Pressão atmosférica, para usar como referencia para sensor de pressão, quando o sensor não for diferenical */
  int32_t pressaoBarometrica = 1013; 
#endif
