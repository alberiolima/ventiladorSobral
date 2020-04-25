/*
 * Rotina para teste do sensor MPXV7002DP +/-2KPa, +/-20,3943cmH2O (analógico)
 */

int16_t tara = 0;

void setup() {
  pinMode( A0, INPUT );
  Serial.begin(115200);
  delay(300);
  for (uint8_t i = 0; i < 10; i++){
    tara += analogRead(A0) - 512;
  }
  tara /= 10;
  
}

void loop() {
  int16_t valorLido = (analogRead(A0) - 512) - tara;
  float valorPa =  (2000.0/512) * (float)(valorLido);
  Serial.print("RAW: ");
  Serial.print(valorLido);
  Serial.print("\tPressão: ");
  Serial.print(valorPa);
  Serial.print("Pa, \t");
  Serial.print(valorPa * 0.010197162129779283);
  Serial.print("cmH2O");
  Serial.println();
  delay(250);
}
