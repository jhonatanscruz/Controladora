// ------------------------ LIBRARIES ------------------------
#include "FTTech_SAMD51Clicks.h"
#include <Keypad.h> // Biblioteca do codigo
#include "FTTech_Components.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "RTC_SAMD51.h"
#include "DateTime.h"
#include<SPIMemory.h>

// ------------------------ DEFINES ------------------------
#define ENCODER_NUMBER_POS 10

// ------------------------ KEYPAD DECLARATION ------------------------

const byte LINHAS = 4; // Linhas do teclado
const byte COLUNAS = 4; // Colunas do teclado

char TECLAS_MATRIZ[LINHAS][COLUNAS] = { // Matriz de caracteres (mapeamento do teclado)
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte PINOS_LINHAS[LINHAS] = {12,6,0,1}; // Pinos de conexao com as linhas do teclado
byte PINOS_COLUNAS[COLUNAS] = {4,46,41,42}; // Pinos de conexao com as colunas do teclado

Keypad myKeypad = Keypad(makeKeymap(TECLAS_MATRIZ), PINOS_LINHAS, PINOS_COLUNAS, LINHAS, COLUNAS); // Inicia teclado

// ------------------------ LCD DECLARATION ------------------------
 
LiquidCrystal_I2C lcd(0x27,16,2);

// ------------------------ STEPPER MOTOR DECLARATION ------------------------
const byte pinPUL = 5;
const byte pinDIR = 44;
const byte pinENA = 47;
const uint16_t microsteps = 1600;
FT_Stepper motor(pinPUL, pinDIR, pinENA, microsteps);

// ------------------------ ENCODER DECLARATION ------------------------
const byte pinA = 10;
const byte pinB = 51;
const byte pinZ = 11;
FT_Encoder encoder(pinA, pinB, pinZ);

// ENCODER VECTOR
/*
  Its a map of encoder's position on each valve
*/
static const int ENCODER_POSITION[ENCODER_NUMBER_POS] = {0,-400,-800,-1190,-1585,-1980,-2390,-2780,-3190,-3597};

// ------------------------ RTC DECLARATION ------------------------
RTC_SAMD51 rtc;

// ------------------------ SPI MEMORY DECLARATION ------------------------

SPIFlash flash(A6);
uint16_t timeLeft = 0;

// ------------------------ GLOBAL VARIABLES ------------------------
uint8_t valv = 1;
bool onZero = false;
uint8_t screen = 1;
bool pression = true;

// ------------------------ SETUP ------------------------
void setup() {

  systemBegin();

  systemSetup();

  if(needConfig(flash)){
    //Apago outras partes da memória
    flash.eraseSector(0); // Tempos de válvulas
    flash.eraseSector(8192); // Tempos restantes de válvulas
    // Solicita que o usuário selecione o tempo de funcionamento de cada setor
    setTime();
  }
  
 // printMenu();
}

// ------------------------ LOOP ------------------------
void loop() {

  valv = flash.readByte(4096); // Recupera da memória o número da válvula

  // Bomba Ligada => Há pressão
  if(pression){

    goToValv(valv); // Motor gira até a válvula

    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("Sistema Ligado");
    lcd.setCursor(3,1);
    lcd.print("Valvula: ");
    lcd.setCursor(12,1);
    lcd.print(valv);

    if(flash.readByte(8192 + (2*(valv-1))) == 255){ // Verifico se a válvula tem tempo restante ou está iniciando agora [Se == 255 então está iniciando agora]
      Serial.println("MEMORY GET TIME FUNCTION [ valv " + (String)valv + "]: " + (String)memoryGetTime(flash, valv, 0));
      keepTime(memoryGetTime(flash, valv, 0)); // Está iniciando agora, então pego o tempo que o usuário definiu
    }
    else{
      Serial.println("MEMORY GET TIME REMAINING [ valv " + (String)valv + "]: " + (String)memoryGetTime(flash, valv, 8192));
      int remaining = memoryGetTime(flash, valv, 8192);
      flash.eraseSector(8192); // Apago da memória o tempo restante
      keepTime(remaining);     // Não está iniciando agora, então pego o tempo RESTANTE salvo na memória quando o sistema foi interrompido
    }

    if(pression){ // Se a bomba continua ligada, então, após o tempo na válvula, passo para a válvula seguinte
      if(valv < 5)
        valv++;
      else
        valv = 1;
    }

    flash.eraseSector(4096); // Apago da memória o número da válvula atual
    flash.writeByte(4096, valv); // Salvo na memória o número da próxima válvula a ser ligada

  }

  // Bomba Desligada => Não há pressão
  else{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Bomba desligada!");

    while(true){ // Aguardo até a bomba ser ligada
      char readKeypad = myKeypad.getKey(); // Faço a leitura do teclado ====== LINHA UTILIZADA MERAMENTE PARA TESTE ======
      if(pression) break; // Se há pressão então quebro o while
      switch(readKeypad){
        case 'D':               // ========== UTILIZAR FUTURAMENTE LEITURA DO SENSOR DE PRESSÃO ==========
          pression = !pression;
          Serial.println("Apertou D");
          break;

        case 'C':     // ========== RESETAR O SISTEMA ==========
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Deseja resetar o");
          lcd.setCursor(0,1);
          lcd.print("sistema inteiro?");

          bool endWhile = false; // Variável auxiliar para sair do While
          while(true){
            char readKeypad = myKeypad.getKey();
            switch(readKeypad){
              case 'A':
                resetSystem();
                endWhile = true;
                if(pression){
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Sistema Ligado!!");
                  lcd.setCursor(3,1);
                  lcd.print("Valvula: ");
                  lcd.setCursor(12,1);
                  lcd.print(valv);
                }

                else{
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Bomba desligada!");
                }
                break;

              case 'B':
                endWhile = true;
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Bomba desligada!");
                break;
            }
            if(endWhile) break;
          }

          break;
      }
    }
  }
}
