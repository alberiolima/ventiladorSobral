/*
   Firmware para teste do sensor BMP/BME280 como sensor de pressão.
   
   A medida da pressão é feita por diferença da pressão
   É necessário um sensor diferencial, dois sensores, ou emular dois sensores
   No caso desse sensor, será necessário, emular, é necessário pegar o valor de 
   pressão antes da operação, quando o dispositivo não estiver conectado ao paciente,
   ou determinar um valor de pressão atmosférica fixo.   
*/
#include <BME280I2C.h>
#include <Wire.h>

#define LCD1602_I2C
#define hPa_mmH2O (10.197162129779283)
#define hPa_cmH2O (1.0197162129779283)
#define Pa_cmH2O (0.010197162129779283)

#ifdef LCD1602_I2C
  /* Objeto do display 16x2*/
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);
#endif

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

BME280I2C bme(settings);

float temperatura(NAN);
float umidade(NAN);
float pressao(NAN);
float pressaoAtmosferica(NAN);

void setup() {

  Wire.begin();
  while (!bme.begin()) {
    Serial.println("Could not find BME280I2C sensor!");
    delay(1000);
  }

  #ifdef LCD1602_I2C
    /* Inicializa LCD1602 */
    lcd.init();      
    
    /* Tela de leituras */
    lcd.clear();
    lcd.setCursor(0, 0) ; lcd.print(F("Pressao cmH2O"));
    lcd.backlight();
  #endif

  // bme.chipID(); // Deprecated. See chipModel().
  switch (bme.chipModel()) {
    case BME280::ChipModel_BME280:
      Serial.println("Found BME280 sensor! Success.");
      break;
    case BME280::ChipModel_BMP280:
      Serial.println("Found BMP280 sensor! No Humidity available.");
      break;
    default:
      Serial.println("Found UNKNOWN sensor! Error!");
  }

  //Change some settings before using.
  settings.tempOSR = BME280::OSR_X4;
  bme.setSettings(settings);

  /* Lê o valor inicial da pressao atmosférica para emular um sensor adicional */
  bme.read( pressaoAtmosferica, temperatura, umidade, BME280::TempUnit_Celsius, BME280::PresUnit_Pa);
  
  /* Inicia serial comunicação serial */
  Serial.begin( 115200 );
  delay(2000);

}

void loop() {
  
  /* faz leitura da pressão, temperatura e umidade[BMP280 não tem umidade] */
  bme.read(pressao, temperatura, umidade, BME280::TempUnit_Celsius, BME280::PresUnit_Pa);

  /* Calcula diferença de pressão e converte para cmH2O */
  float pressaoCmH2O = (pressao - pressaoAtmosferica) * Pa_cmH2O;
  
  #ifdef LCD1602_I2C 
    /* Mostra valor no LCD */
    lcd.setCursor(0, 1); 
    lcd.print(pressaoCmH2O,1);
    lcd.print("   ");
  #endif
  
  /* Envia dados para serial */
  Serial.print( pressaoAtmosferica );
  Serial.print( "\t" );
  Serial.print(pressao);
  Serial.print( "\t" );
  Serial.print(pressao-pressaoAtmosferica);
  Serial.print( "\t" );
  Serial.print(pressaoCmH2O);
  Serial.println();
  
  /* Tempo entre leituras */
  delay(250);

}
