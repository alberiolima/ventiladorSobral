#ifndef _TIPOS_DEFS_H
#define _TIPOS_DEFS_H

/* Definição de erros/alarmes */
enum {
  statusOK,
  statusErroNaoDefinido,
  StatusSemPressao,
  StatusSemVolume
};

/* Tipos de controle da ventilação */
enum {
  controlePorPressao,
  controlePorVolume
};

/* Numeração dos potenciômetros */
enum {
  potVolumeCorrente,  
  potFrequencia,
  potOxigenio,
  potPressaoPlato,  
  potRelacaoIE,    
  potPEEP,  
  //potTrig,  
  quantidadePotenciometros
};

/* Numeração das telas de status dinâmico */
enum {
  telaStatusTempo,
  telaStatusPressaoInspiracao,
  telaStatusVolumeExpiracao,
  telaStatusVolumeInspiracao,
  telaStatusParametros,
  telaStatusIE,
  quantidadeTelasStatus
};

/* Parâmetros de operação, envia para módulo [operação] */
typedef union {
  struct {
    uint8_t tipoControle = controlePorPressao;
    int32_t pressaoDePlato = 300; //em mmH2O
    int32_t pressaoDePico = 320;  //pressaoDePlato + 20;
    int32_t pressaoPEEP = 50;
    uint32_t volumeCorrente = 400;
    uint32_t tempoInspiracao = 1000; //Tempo de inspiração em ms
    uint32_t tempoExpiracao  = 2000; //Tempo de expiração em ms (calculado com base na razão I:E)
  } __attribute__((packed));
  uint8_t raw[25];    
} parametros_t; 

/* Dados da ultima respiração executada, recebe do módulo [operação] */
typedef union {
  struct {
    int32_t pressaoInspiracao = 0; //mmH2O
    int32_t pressaoPEEP = 0;       //mmH2O
    uint32_t volumeInspiracao = 0;  //ml
    uint32_t volumeExpiracao = 0;   //ml
    uint32_t tempoInspiracao = 0;   //ms
    uint32_t tempoExpiracao = 0;    //ms
    uint8_t status = statusOK;
  } __attribute__((packed));
  uint8_t raw[25];
} status_t;
#endif
