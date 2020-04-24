/*
 *  O que precisa ser controlado
 *  # Tipo    [Mandatório,Assistido] (chave)
 *  # Conrole [Pressão,Volule] (chave)
 *  > Volume Corrente
 *  > Pressão de platô [20-30]cmH2O
 *  > Frequência [12-30]Hz
 *  > Relação I:E 
 *  > Oxigencio [0,100]% *  
 *  > PEEP 0-5cmH2O
 *  > Trigger (gatilho) [0 a 5]cmH2O 
 *  * o calculo de tempo de inspiração e expiração é calculado com base na frequencia e relação I:E
 */
//#include <avr/wdt.h> //Biblioteca que cão de guarda (WDT)
#include "config.h"
#include "tiposDefs.h"
#include "debug.h"
#include "funcoes.h"

/* Cabelaho com instruções de inicialização, criado apenas para deixar mais legível o código */
#include "main.h"

/* Variáveis de controle dos parâmetros */
const uint8_t portasPotenciometro[quantidadePotenciometros] = {PORTA_POT1,PORTA_POT2,PORTA_POT3,PORTA_POT4,PORTA_POT5,PORTA_POT6};
int16_t valorParametro[quantidadePotenciometros];     //Valor do parâmetro, esse valor que será usado na operação
int16_t valorPotenciometro[quantidadePotenciometros]; //Valor raw lido da porta analógica
int16_t valorIncremento[quantidadePotenciometros];    //Incremento/decremento do valor
int16_t valorMAX[quantidadePotenciometros];           //Valor máximo do parâmetro
int16_t valorMIN[quantidadePotenciometros];           //Valor mínimo do parâmetro
String titulo[quantidadePotenciometros];              //Titulo do parâmetro
String unidades[quantidadePotenciometros];            //Unidade de medida do parâmetro

/* Parâmetros da respiração, IHM envia para [operação] */
parametros_t parametrosRespiracao;

/* Status, que o IHM recebe da [operação] */ 
status_t statusIHM;

/* volume em ml*/
uint32_t volumeAtual = 0;

/* pressão em mmH2O, usando mmH2O para não ter valores fracionados nos calculos, ex: 25mmH2O = 2.5cmH2O */
int32_t pressaoAtual = 0;

uint8_t statusPower = !POWER_ON;  //Status de operação, indica posição da chave que inicia/para operação
uint32_t tempoInicioOperacao = 0; //Para contar o tempo duração operação,
uint32_t proximoPiscaLCD = 0;     //Controla o pisca do LCD quando em alarme
boolean zoomHabilitado = true;    //Habilita efeito de mostrar um parâmetro por vez quando o poteciômetro é acionado

/* funções locais */
void telaParametros( boolean limpar = false );  //Tela com todos os parâmetros
void telaParametro( uint8_t p );                //Tela com um unico parâmetro em zoom
void MostraTelaStatus( boolean force = false ); //Status rotativo quando em operação
void lePotenciometros(boolean force = false);   //faz leitura dos potenciômetros

void setup() {
  
  /* Inicializa LCD1602 */
  lcd.init();  

  /* Inicia portas IOs */
  iniciaIOs();
 
  /* Inicia porta de debug */
  #if defined(debbug_on)
    Serial.begin( debbug_baud );
    delay(300); //Tempo necessário para atmega32u4 iniciar serial
    Serial.println(F("Iniciado"));
  #endif  

  /* Verifica estado no botão PowerON, deve estar desligado quando o IHM é ligado */
  if (digitalRead( PINO_POWER ) == LOW ) {
    delay(10);
    if (digitalRead( PINO_POWER ) == LOW ) {
      DBG_PRINTLN(F("IHM reiniciado"));
      //soar alarme
      proximoPiscaLCD = 1;
    }    
  }

  /* Inicia sensores, presão, fluxo, etc */
  iniciaSensores();
  
  /* Inicia valores dos parâmetros */
  iniviaValores();
  statusPower = digitalRead( PINO_POWER );  

  /* Inicia botão de mudança de Status */
  btStatus.begin();

  /* Inicia parâmetros para respiração */
  atualizaParametros();

  /* Atualiza dados dos sensores */
  atualizaDadosSensores();

  /* Tela de boas vindas */
  lcd.clear();
  lcd.setCursor(0, 0) ; lcd.print(F("Boas Vindas"));
  lcd.backlight();
  delay(2000);  

  /* Inicia com a tela de parâmtros */
  telaParametros(true);  

  /* Ativa WDT */
  //wdt_enable(WDTO_4S);
}

void loop() {

  /* Reinicia contador do WDT */
  //wdt_reset();

  /* Pisca LCD (alarme), quando ativado */
  if ((proximoPiscaLCD > 0)&&(millis()>proximoPiscaLCD)) {
    proximoPiscaLCD = millis() + (uint32_t)1000;
    lcd.noBacklight();
    delay(100);
    lcd.backlight();
  }

  /* Inicia ou para operação, quando a chave power estiver acionada */
  verificaPower();   

  /* Mostra status */
  if ( statusPower == POWER_ON ) { //Em operação

    /* Mostra tela dinâmica de stautus quando em operação */
    if (btStatus.pressed()){
      MostraTelaStatus(true);
    } else { 
      MostraTelaStatus();
    }
    
    /* Executa respiração */
    respira(); 
   
  } else { //Em configuração
    /* Habilita ou desabilita o zoom do parâmetro, quando botão status for acionado */
    if (btStatus.pressed()){
      zoomHabilitado = !zoomHabilitado;
      if (!zoomHabilitado){
        telaParametros(true);
      }
    }
    
    /* Faz a leitura dos potenciometros, um por ciclo do loop */
    lePotenciometros();
  }
  
}

void atualizaParametros() {
  
  /* Atualiza parâmetros da respiração de acordo com os dados dos potenciômetros */
  parametrosRespiracao.tipoControle = controlePorPressao;
  parametrosRespiracao.pressaoDePlato = (int32_t)valorParametro[potPressaoPlato] * 10;
  parametrosRespiracao.pressaoDePico = (parametrosRespiracao.pressaoDePlato + (int32_t)2) * 10;
  parametrosRespiracao.pressaoPEEP = (int32_t)valorParametro[potPEEP] * 10;
  parametrosRespiracao.volumeCorrente = (uint32_t)valorParametro[potVolumeCorrente];

  /* Calcula tempo de inspiração e expiração */
  float tempoInsp = (600.0 / (float)valorParametro[potFrequencia])/(10.0 + (float)valorParametro[potRelacaoIE]) * 1000.0;
  parametrosRespiracao.tempoInspiracao = (uint32_t)tempoInsp;
  parametrosRespiracao.tempoExpiracao  = (parametrosRespiracao.tempoInspiracao * (uint32_t)valorParametro[potRelacaoIE])/(uint32_t)10;
}

/* Faz leitura dos potenciômetros */
void lePotenciometros(boolean foce){
  static uint32_t proximaLeituraPotenciometro = 0;
  static uint32_t tempoParametroIndividual = 0;
  if ((!foce)&&( millis() < proximaLeituraPotenciometro )) {
    return;
  }
  proximaLeituraPotenciometro = millis() + (uint32_t)50;
  if ((zoomHabilitado)&&(tempoParametroIndividual>0)&&(millis()>tempoParametroIndividual)){
    tempoParametroIndividual = 0;
    telaParametros(true);
  }

  /* Efetua a leitura do potenciômetro */
  static uint8_t potenciometroAtual = 0;
  int16_t leituraPot = analogRead(portasPotenciometro[potenciometroAtual]);
  
  /* Atualiza o valor apenas se for diferente da leitura anterior */
  if ( leituraPot != valorPotenciometro[potenciometroAtual]){
    valorPotenciometro[potenciometroAtual] = leituraPot;
    int16_t valorP = map( leituraPot, ANALOG_MIN, ANALOG_MAX, valorMIN[potenciometroAtual],valorMAX[potenciometroAtual]);
    int16_t valorI = (valorP / valorIncremento[potenciometroAtual]);
    if ( valorI == 0 ) {
      valorP = valorMIN[potenciometroAtual];
    } else {
      valorP = valorI * valorIncremento[potenciometroAtual];
    }
    if ( valorP != valorParametro[potenciometroAtual]){
      valorParametro[potenciometroAtual] = valorP;        
      if (zoomHabilitado) {
        tempoParametroIndividual = millis() + (uint32_t)3000;
        telaParametro(potenciometroAtual);
      } else {
        telaParametros();
      }
    }    
  }
  
  /* Atualiza próximo potenciômetro e ser lido */
  potenciometroAtual++;
  if ( potenciometroAtual >= quantidadePotenciometros ) { 
    potenciometroAtual = 0;
  }  
}

void telaParametro( uint8_t p ) {
  static uint8_t pAnterior = 0;
  if ( p == 100 ) {
    pAnterior = 100;
    return;
  } else if ( pAnterior != p ){
    pAnterior = p;
    lcd.clear();
    lcd.setCursor( 0, 0 ) ;
    lcd.print( titulo[p] );
  }
  lcd.setCursor( 0, 1 ); 
  lcd.print(intToStrSpace(valorParametro[p],4));
  lcd.print(unidades[p]);
}

void telaParametros( boolean limpar ){
  if (limpar){
    lcd.clear();
  }    
  
  telaParametro(100);
  
  /* Linha 1 do LCD1602 */
  lcd.setCursor( 0, 0); lcd.print(intToStrSpace(valorParametro[potVolumeCorrente],4));
  lcd.setCursor( 8, 0); lcd.print(valorParametro[potFrequencia]);
  lcd.setCursor(13, 0); lcd.print(intToStrSpace(valorParametro[potOxigenio],3));
  
  /* Linha 2 do LCD1602 */
  lcd.setCursor( 0, 1); lcd.print(intToStrSpace(valorParametro[potPressaoPlato],4));
  lcd.setCursor( 6, 1); lcd.print("1:");lcd.print((float)valorParametro[potRelacaoIE]/10.0,1);
  lcd.setCursor(14, 1); lcd.print(intToStrSpace(valorParametro[potPEEP],2));    
}

/* Inicia variáveis de controle dos potenciômetros, MAX,MIN e Incremento */
void iniviaValores() {

  /* Volume corrente */  
  titulo[potVolumeCorrente] = F("Volume corrente");
  unidades[potVolumeCorrente] = F("ml   ");
  valorIncremento[potVolumeCorrente] = 50;
  valorMIN[potVolumeCorrente] = 250;  
  valorMAX[potVolumeCorrente] = 800;

  /* Pressão de platô */
  titulo[potPressaoPlato] = F("Pressao plato");
  unidades[potPressaoPlato] = F("cmH2O");
  valorIncremento[potPressaoPlato] = 5;
  valorMIN[potPressaoPlato] = 5;  
  valorMAX[potPressaoPlato] = 35;
  
  /* Frequencia */
  titulo[potFrequencia] = F("Frequencia");
  unidades[potFrequencia] = F("Ciclo/min");
  valorIncremento[potFrequencia] = 2;
  valorMIN[potFrequencia] = 0;  
  valorMAX[potFrequencia] = 30;
  
  /* Relação I:E*/
  titulo[potRelacaoIE] = F("Relecao I/E");
  unidades[potRelacaoIE] = F("     ");
  valorIncremento[potRelacaoIE] = 5;
  valorMIN[potRelacaoIE] = 10;
  valorMAX[potRelacaoIE] = 30;
  
  /* Oxigenio */
  titulo[potOxigenio] = F("Oxigencio");
  unidades[potOxigenio] = F("%     ");
  valorIncremento[potOxigenio] = 5;
  valorMIN[potOxigenio] = 20;  
  valorMAX[potOxigenio] = 100;
  
  /*PEED */
  titulo[potPEEP] = F("PEEP");
  unidades[potPEEP] = F("cmH2O");
  valorIncremento[potPEEP] = 5;
  valorMIN[potPEEP] = 0;  
  valorMAX[potPEEP] = 25;
  
  //potTrig,  
  
  /* Inicia valores dos parâmetros valorParametro[x]*/
  DBG_PRINTLN(F("Valores dos potenciômetros"));
  for ( uint8_t i = 0; i < quantidadePotenciometros; i++){
    valorPotenciometro[i] = 1025;
    lePotenciometros(true);
    DBG_PRINT(F("pot"));DBG_PRINT(i+1);DBG_PRINT(F(":"));DBG_PRINTLN(valorParametro[i]);    
  }  
}

/* Mostra status rotativo, quando em operação */
void MostraTelaStatus( boolean force ){
  static uint32_t proximoStatus = 0;  
  static uint8_t statusAtual = 0;
  /* status sem tempo de espera */
  if ( statusAtual == 0 ) {
    uint32_t tempoDecorrido = (millis() - tempoInicioOperacao)/(uint32_t)1000;
    lcd.setCursor( 0, 1);
    lcd.print(formatTime(tempoDecorrido));
    if ( tempoDecorrido >= (uint32_t)86400 ) {
      tempoDecorrido = tempoDecorrido / (uint32_t)86400;
      lcd.print( F(" dias:") );
      lcd.print( tempoDecorrido );
    }    
  } else if (statusAtual == 1) {
    lcd.setCursor( 0, 0);
    lcd.print(F("Pressao Inps"));
    lcd.setCursor( 0, 1);
    lcd.print(statusIHM.pressaoInspiracao/10.0);    
  }
  if ((!force)&&(proximoStatus > millis())){
    return;
  }
  
  /* Atualiza status rotativo */
  statusAtual++;
  if ( statusAtual >= quantidadeTelasStatus ) {
    statusAtual = 0;
  }
  proximoStatus = millis() + (uint32_t)TEMPO_STATUS;
  
  lcd.clear();
  String linha1LCD = "";
  String linha2LCD = "";
  if ( statusAtual == telaStatusTempo) {
    /* Tela que mostra tempo decorrido da operação */
    linha1LCD = F("Tempo Decorrido");
  } else if ( statusAtual == telaStatusPressaoInspiracao){
    /* Tela que mostra a pressão final da última inspiração */
    linha1LCD = F("Pressao Inps");
    linha2LCD = String(statusIHM.pressaoInspiracao / 10.0);
  } else if ( statusAtual == telaStatusVolumeInspiracao){
    /* Tela que mostra o volume final da última inspiração */
    linha1LCD = F("Volume Inspracao");
    linha2LCD = String(statusIHM.volumeInspiracao);    
  } else if ( statusAtual == telaStatusVolumeExpiracao){
    /* Tela que mostra o volume final da última expiração */
    linha1LCD = F("Volume Expiracao");
    linha2LCD = String(statusIHM.volumeExpiracao);
  } else if ( statusAtual == telaStatusParametros ){
    /* Tela com todos os parâmetros de configuração */
    telaParametros();
    return;
  } else if ( statusAtual == telaStatusIE ) {
    /* Tela que mostra tempos de inspiração e expiração da última respiração */
    linha1LCD = F("Tempo Insp/Exp");
    linha2LCD = String(statusIHM.tempoInspiracao/1000.0) + " " +String(statusIHM.tempoExpiracao/1000.0);
  }
  /* Escreve dados o LCD */
  lcd.setCursor( 0, 0); lcd.print(linha1LCD);
  lcd.setCursor( 0, 1); lcd.print(linha2LCD);
}

void iniciaIOs(){
  
  /* Inicia botão power on/off */
  #ifdef debbug_on
    pinMode( PINO_POWER, INPUT_PULLUP );
  #else
    pinMode( PINO_POWER, INPUT ); //Adicionar resistor de pull-up
  #endif

  /* inicia valvula de inspiração */
  #ifdef PINO_VALVULA_INSPIRACAO
    pinMode( PINO_VALVULA_INSPIRACAO, OUTPUT );
    fechaValvulaInspiracao(); //inicia com valvula da inspirção fechada
  #endif

  /* inicia valvula de expiração */
  #ifdef PINO_VALVULA_EXPIRACAO
    pinMode( PINO_VALVULA_EXPIRACAO, OUTPUT );
    fechaValvulaExpiracao(); //inicia com valvula da inspirção fechada
  #endif
   

  #ifdef PINO_ALARME
    pinMode( PINO_ALARME, OUTPUT );
  #endif

  /* Inicia portas analógicas de entrada, para os potenciômetros */  
  for ( uint8_t i = 0; i < quantidadePotenciometros; i++){
    pinMode( portasPotenciometro[i], INPUT);
  }
}

void respira(){
  /* Inspiração */
  pressaoAtual = 0;
  volumeAtual = 0;
  abreValvulaInspiracao(); //Abre válvula da inspiração 
  uint32_t tempoAtual = millis();
  uint32_t finalInspiracao = tempoAtual + parametrosRespiracao.tempoInspiracao; //Agenda final da inspiração  
  //wdt_reset();
  if ( parametrosRespiracao.tipoControle == controlePorPressao ) {
    while ((tempoAtual < finalInspiracao)&&(pressaoAtual < parametrosRespiracao.pressaoDePlato)) {      
      atualizaDadosSensores();
      tempoAtual = millis();      
    }
    statusIHM.pressaoInspiracao = pressaoAtual;
    statusIHM.volumeInspiracao = volumeAtual;
    if ( pressaoAtual < parametrosRespiracao.pressaoDePlato ) {
      //não antingiu a pressão necessária, aumentar vazão
    }
  } else if ( parametrosRespiracao.tipoControle == controlePorVolume ) {
    while (( tempoAtual < finalInspiracao )&&( volumeAtual < parametrosRespiracao.volumeCorrente )&&( pressaoAtual < parametrosRespiracao.pressaoDePico )) {
      atualizaDadosSensores();
      tempoAtual = millis();
    }
    if ( volumeAtual < parametrosRespiracao.volumeCorrente ) {
      //não antingiu o volume necessário, aumentar vazão
    }
  }  
  fechaValvulaInspiracao();// fecha válvula da inspiração  
  if ( tempoAtual < finalInspiracao ) {
    //interrompido antes do final do tempo, possivelmente atingiu a pressão/volume antes do tempo
    //ajustar diminuindo vazão
  }  
  statusIHM.tempoInspiracao = millis() - (finalInspiracao - parametrosRespiracao.tempoInspiracao);  

  /* Expiração */
  volumeAtual = 0;
  abreValvulaExpiracao(); // Abre valvula da expiração
  tempoAtual = millis();
  uint32_t finalExpiracao = tempoAtual + parametrosRespiracao.tempoExpiracao;  
  while (( tempoAtual < finalExpiracao ) 
  #ifdef CONTROLE_PEEP 
    &&(pressaoAtual > parametrosRespiracao.pressaoPEEP )
  #endif
  ) {
    //wdt_reset();
    atualizaDadosSensores();
    tempoAtual = millis();    
  }
  fechaValvulaExpiracao(); // Fecha valvula da expiração
  statusIHM.volumeExpiracao = volumeAtual;
  statusIHM.pressaoPEEP = pressaoAtual;
  //wdt_reset();
  while ( tempoAtual < finalExpiracao ){ //Aguarda concluir tempo de expiração
    atualizaDadosSensores();
    tempoAtual = millis();
  }
  #ifdef CONTROLE_PEEP
    if (pressaoAtual < parametrosRespiracao.pressaoPEEP){
      //Verificar porque o PEEP não foi atingido
    }      
  #endif
  statusIHM.tempoExpiracao = millis() - (finalExpiracao - parametrosRespiracao.tempoExpiracao);  
}

void atualizaDadosSensores(){
  static uint32_t lerPressao = 0;
  pressaoAtual = 100; //Leitura do sensor de pressão
  volumeAtual = 100;
  if ( millis() > lerPressao ) {
    lerPressao = millis() + (uint32_t)50;
    #ifdef SENSOR_BMP180
      sensorPressaoBMP180.temperatureStartUpdate();
      delay(5);
      sensorPressaoBMP180.temperatureUpdate();

      sensorPressaoBMP180.pressureStartUpdate();
      delay( sensorPressaoBMP180.timeConversionPressure );
      sensorPressaoBMP180.pressureUpdate();
      pressaoAtual = (int32_t)((float)( sensorPressaoBMP180.pressure - pressaoBarometrica ) * hPa_mmH2O);
    #endif
    #ifdef SENSOR_BMP280
      float temp(NAN), hum(NAN), pres(NAN);
      sensorPressaoBMP280.read(pres, temp, hum, BME280::TempUnit_Celsius, BME280::PresUnit_Pa); //PresUnit_hPa
      pressaoAtual = (int32_t)((float)( pres - (float)pressaoBarometrica ) * Pa_mmH2O);
    #endif 
  } 
  MostraTelaStatus(); //verificar se ficando aqui não projudica a execução
}

void verificaPower() {
  /* Verifica botão PowerOn (inicia/finaliza operação) */
  uint8_t p = digitalRead( PINO_POWER );
  if ( statusPower != p ){
    delay(10);
    uint8_t p2 = digitalRead( PINO_POWER );
    if ( p == p2 ) {
      statusPower = p;  
      if ( statusPower == POWER_ON ) {
        tempoInicioOperacao = millis();
        proximoPiscaLCD = 0;
        atualizaParametros();
        DBG_PRINTLN(F("Operação iniciada"));
      } else  {        
        telaParametros(true);
        DBG_PRINTLN(F("Operação finalizada"));
      }
    }    
  }
}

void iniciaSensores(){
  #ifdef SENSOR_BMP180
    if (!sensorPressaoBMP180.begin()){
      DBG_PRINTLN(F("BMP180 não iniciado"));
    } else {
      for ( byte i = 0; i < 10; i++){
        sensorPressaoBMP180.temperatureStartUpdate();
        delay(5);
        sensorPressaoBMP180.temperatureUpdate();

        sensorPressaoBMP180.pressureStartUpdate();
        delay( sensorPressaoBMP180.timeConversionPressure );
        sensorPressaoBMP180.pressureUpdate();
        if ( i == 0 )  {
          pressaoBarometrica = sensorPressaoBMP180.pressure;
        } else{
          pressaoBarometrica = (pressaoBarometrica + sensorPressaoBMP180.pressure)/2;
        }
      }
      DBG_PRINT(F("Pressão barométrica BMP180: "));
      DBG_PRINTLN(pressaoBarometrica);
    }
  #endif  
  #ifdef SENSOR_BMP280  
    Wire.begin();
    if (!sensorPressaoBMP280.begin()) {
      DBG_PRINTLN(F("BMP280 não iniciado"));
    } else {
      // bme.chipID(); // Deprecated. See chipModel().
      switch (sensorPressaoBMP280.chipModel()) {
        case BME280::ChipModel_BME280:
          DBG_PRINTLN(F("Found BME280 sensor! Success."));
          break;
        case BME280::ChipModel_BMP280:
          DBG_PRINTLN(F("Found BMP280 sensor! No Humidity available."));
          break;
        default:
          DBG_PRINTLN(F("Found UNKNOWN sensor! Error!"));
      }

      // Change some settings before using.
      settings.tempOSR = BME280::OSR_X4;
      sensorPressaoBMP280.setSettings(settings);
      float temp(NAN), hum(NAN), pres(NAN);
      sensorPressaoBMP280.read(pres, temp, hum, BME280::TempUnit_Celsius, BME280::PresUnit_hPa);
      pressaoBarometrica = (int32_t)pres;
      for ( byte i = 0; i < 9; i++){
        delay(100);
        sensorPressaoBMP280.read(pres, temp, hum, BME280::TempUnit_Celsius, BME280::PresUnit_Pa); //PresUnit_hPa
        pressaoBarometrica = (pressaoBarometrica + (int32_t)pres)/2;
      }
      DBG_PRINT(F("Pressão barométrica BMP280: "));
      DBG_PRINT(pressaoBarometrica);
      DBG_PRINT(F(" "));
      DBG_PRINT((float)pressaoBarometrica*Pa_mmH2O);
      DBG_PRINTLN(F("mmH2O"));      
    }  
  #endif  
  
}
