/* 
  Função que format a data/hora de UNIX para uma string hh:mm:ss, desconsiderando os dias 
*/
String formatTime( uint32_t t) {
  static const uint32_t segundosDia = 86400;
  static const uint32_t segundosHora = 3600;
  static const uint32_t segundosMinuto = 60;
  String st = "";

  uint32_t tt = 0;

  //retira os dias
  if ( t >= segundosDia ) {
    tt = ( t / segundosDia );
    t -= ( tt * segundosDia );
  }

  //hora
  if ( t >= segundosHora ) {
    tt = (t / segundosHora);
    t -= ( tt * segundosHora);
    if ( tt < (uint32_t)10 ) {
      st += "0";
    }
    st += String(tt) + ":";
  } else {
    st += "00:";
  }

  //minutos
  if ( t >= segundosMinuto ) {
    tt = (t / segundosMinuto);
    t -= (tt * segundosMinuto);
    if ( tt < (uint32_t)10 ) {
      st += "0";
    }
    st += String(tt) + ":";
  } else {
    st += "00:";
  }

  //segundos
  if ( t < (uint32_t)10 ) {
    st += "0";
  }
  st += String(t);

  return (st);
}

String intToStrSpace( int valor, uint8_t len ) {
  String retorno = String(valor);
  while (retorno.length()< len){
    retorno = " " + retorno;
  }
  return retorno;
}
