// =================================================================================
// ================================ GENERAL FUNCTIONS ==============================
// =================================================================================

// Transforma um dígito inteiro declarado como CHAR para o tipo INT
int charToInt(char c){
  int base = (int)'0';
  int num = (int)c;
  return (c - base);
}

// ------------------------------------------------------------------------
// Transforma um número inteiro declarado como STRING para o tipo INT
int strToInt(String str){

  int numb = 0;
  for(int i = 0; i < str.length() ; i++){
    numb = numb + (charToInt(str[i]) * pot(10,str.length()-(i+1)));
  }
  return numb;
}

// ------------------------------------------------------------------------
// Realiza a operação de potênciação dados a base e o expoente
int pot(int base, int expo){
  int result = 1;
  for(int i = 0 ; i < expo ; i++){
      result = result * base;
  }
  return result;
}

//---------------------------------------------------------------------------------

// =================================================================================
// ================================ ENCODER FUNCTIONS ==============================
// =================================================================================

void PinA_OnChange(){
  encoder.PinA_OnChange();
}

// ------------------------------------------------------

void PinB_OnChange(){
  encoder.PinB_OnChange();
}

// ------------------------------------------------------

void PinZ_OnFalling(){
  if(!onZero){
    motor.stop();
    motor.changeDIR(1);
    onZero = true;
  }
}
// ------------------------------------------------------

// =================================================================================
// ============================ PRESSURE SENSOR FUNCTIONS ==========================
// =================================================================================

float fromPressureSensor_GetVoltage(){
  float voltage = (analogRead(PRESSURE_SENSOR_PIN)*5.0)/1024.0;
  return voltage;
}

//----------------------------------------------------------

float fromPressureSensor_GetMPaPressure(){
  //Converts into [MPa]
  float pressure = analogRead(PRESSURE_SENSOR_PIN)* 0.001169;
  return pressure;
}

//----------------------------------------------------------

bool pression(void){
  // System starts after 0.3MPa
  if(fromPressureSensor_GetMPaPressure() > TRANSITION_PRESSURE){
    return true;
  }
  return false;
}

//----------------------------------------------------------

// ============================================================================
// ============================== SYSTEM FUNCTIONS ============================
// ============================================================================

void systemBegin(){
  FTClicks.turnON_5V(); // Enable 5V Output
  FTClicks.turnON(1);  // Step-Up 5-24V Click
  FTClicks.turnON(2);  // Step-Up 5-24V Click
  FTClicks.turnON(3);  // Step-Up 5-24V Click
  lcd.init();
  motor.begin();
  encoder.begin();
  flash.begin();
  rtc.begin();
  DateTime now = DateTime(F(__DATE__), F(__TIME__));
  rtc.adjust(now);
  DateTime initTime = rtc.now(); // Início da contagem
  Serial.begin(DEBUG_BAUDRATE); // Inicia porta serial

  // AttachInterrupt0, digital PinA, Signal A
  // It Activates interruption on any signal change
  attachInterrupt(digitalPinToInterrupt(pinA), PinA_OnChange, CHANGE);

  // AttachInterrupt1, digital PinB, Signal B
  // It Activates interruption on any signal change
  attachInterrupt(digitalPinToInterrupt(pinB), PinB_OnChange, CHANGE);

  //AttachInterrupt2, digital PinZ, Signal Z
  // It Activates interruption at the falling edge of the signal
  attachInterrupt(digitalPinToInterrupt(pinZ), PinZ_OnFalling, FALLING);

  if(DEBUG) Serial.println("Iniciou!");

  initAnimation();
}

// ------------------------------------------------------

// Animação de abertura do sistema
void initAnimation(){
  lcd.clear();
  lcd.setBacklight(1);
  lcd.setCursor(0,0);
  lcd.print("FT Technology");
  for(int i = 0; i < 16; i++){
    lcd.setCursor(i,1);
    lcd.print(".");
    delay(150);
  }
}

// ------------------------------------------------------

// Inicialização do sistema
void systemSetup(){

  onZero = false;
// Set system to zero
  if(!onZero){
    motor.changeDIR(MOTOR_DIRECTION);
    motor.setPulseDelay(MOTOR_PULSE_DELAY); // Change motor velocity
    motor.start();
    while(!onZero) motor.pulse();

    encoder.reset();

    // Offset to adjust the motor position to zero point
    while(encoder.getPosition() > ENCODER_POSITION[1]) motor.pulse();
    
    motor.stop();

    // Now system is in zero point so its parameters should be reseted
    motor.reset();
    motor.changeDIR(MOTOR_DIRECTION); // Motor rotates clockwise
    motor.setPulseDelay(MOTOR_PULSE_DELAY); // Change motor velocity after reset
    encoder.reset();
  
    if(flash.readByte(CURRENT_VALVE_ADRESS) == 255){ // Verifico se já existe uma válvula salva previamente
      flash.writeByte(CURRENT_VALVE_ADRESS, valv); // Caso não exista, salvo na memória a primeira válvula a ser ligada
    }
  }
  
  if(DEBUG) Serial.println("Válvula Atual: " + (String)flash.readByte(CURRENT_VALVE_ADRESS));
}

// ------------------------------------------------------

// Função executada para alterar para a válvula seguinte
void goToValv(uint8_t valv){

  motor.start();

  if(encoder.getPosition() > ENCODER_POSITION[2*(valv -1)]){
    while(encoder.getPosition() > ENCODER_POSITION[2*(valv -1)]){
      motor.pulse();
    }
  }

  else{
    while(encoder.getPosition() < (ENCODER_POSITION[2*(valv-1)]) && encoder.getPosition() < (ENCODER_POSITION[2*(valv-1)] - 5)){
      motor.pulse();
    }
  }

  motor.stop();
}

// ------------------------------------------------------

// Função retorna true caso seja necessário configuração inicial
// Retorna false caso o sistema já esteja configurado
// OBS.: O Sistema não está configurado quando a memória está limpa
bool needConfig(SPIFlash flash){
  if(flash.readByte(VALVE_TIME_SECTOR) != 255){
    return false;
  }

  else{
    return true;
  }
}

// ------------------------------------------------------

// Função que executa a reconfiguração do sistema
void resetSystem(){

  //Apago os blocos de memória
  flash.eraseSector(VALVE_TIME_SECTOR); // Tempos de válvulas
  flash.eraseSector(CURRENT_VALVE_ADRESS); // número da válvula atual
  flash.eraseSector(TIME_LEFT_SECTOR); // Tempo restante na válvula atual

  valv = 1;
  if(DEBUG) Serial.println("Iniciou!");
  initAnimation();
  systemSetup();
  setTime();
}

// ------------------------------------------------------------------------

// ============================================================================
// ============================ SPI MEMORY FUNCTIONS ==========================
// ============================================================================

// Salva na memória o tempo de abertura de cada válvula
void memorySaveTime(uint16_t timing, SPIFlash flash, uint8_t valv, uint16_t firstAddress){

    // Se o tempo for menor ou igual a 254 => Consigo salvar em 1 Byte
  if(timing <= 254){
    for(uint8_t i = 0; i < 9; i = i+2){
      if(i == 2*(valv-1))
      {
        // Escreve na memória
        flash.writeByte(i+firstAddress, 0); // Primeiro Byte recebe 0
        flash.writeByte(i+1+firstAddress, timing); // Segundo Byte recebe o tempo em minutos
      }
    }
  }

  // Se o tempo for superior a 254 => Divido o valor em 2 Bytes
  else{
    for(uint8_t i = 0; i < 9; i = i+2)
    {
      if(i == 2*(valv-1))
      {
        // Operação para separar o valor em 2 Bytes
        uint8_t byte1 = (uint16_t)floor(timing/10); // Primeiro Byte = Parte inteira da divisão por 10 do valor inicial
        uint8_t byte2 = timing - (10*byte1); // Segundo Byte = Parte unitária do valor inicial

        // Escreve na memória
        flash.writeByte(i+firstAddress, byte1);
        flash.writeByte(i+1+firstAddress, byte2);
      }
    }
  }
}

// ------------------------------------------------------------------------
// Recupera os valores salvos anteriormente na memória
uint16_t memoryGetTime(SPIFlash flash, uint8_t valv, uint16_t firstAddress){

  uint16_t timing;
  uint16_t pos1;
  uint16_t pos2;

  // Procuro o tempo salvo para a válvula passada no parâmetro
  for(int i = 0; i < 9; i = i+2){
    if(i == 2*(valv-1)){
      pos1 = flash.readByte(i+firstAddress);
      pos2 = flash.readByte(i+1+firstAddress);
    }
  }
  return (pos1*10)+pos2;
}

// ------------------------------------------------------------------------

// Função de configuração do tempo em que as válvulas ficarão em cada setor
// Função interage com o usuário e salva na memória os valores escolhidos
void setTime(){

  String myTime = ""; // Defino a variável que irá receber o tempo
  bool endWhile = false; // Variável auxiliar para sair do While

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mesmo tempo para");
  lcd.setCursor(2,5);
  lcd.print("os setores?");

  while(true){
    char readKeypad = myKeypad.getKey();
    if(readKeypad){

// -------------------- SETORES COM TEMPOS IGUAIS --------------------

      if(readKeypad == 'A'){
        
        // Já garanti que a memória é apagada previamente

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Digite o tempo:");

        while(true){
          char readKeypad = myKeypad.getKey(); // Faço a leitura do teclado
          if(readKeypad){
            switch(readKeypad){
              case 'A':
                if(myTime.length() > 0){
                  lcd.clear();
                  lcd.setCursor(0,0);
                  lcd.print("Configurando");
                  for(int i = 0; i < 16 ; i++){
                    lcd.setCursor(i,1);
                    lcd.print(".");
                    delay(150);
                  }

                  // Salvo os dados do tempo na memória            
                  for(int i = 1; i <= 5 ; i++){
                    memorySaveTime(strToInt(myTime), flash, i, VALVE_TIME_SECTOR);
                    if(DEBUG) Serial.println("Salvei "+(String)strToInt(myTime)+ " no endereço " + (String)i);
                  }

                  endWhile = true;
                }

                break;

              case 'B': 
              myTime = ""; // Reset myTime

              // Print page again
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("Digite o tempo:");

              case 'C': case 'D': case '*': case '#':
                break;

              default:
                if(myTime.length() < 3){
                  myTime.concat(readKeypad);
                  lcd.setCursor(myTime.length()-1,1);
                  lcd.print(readKeypad);
                }
            }
          }
          if(endWhile) break;
        }
      }

  // -------------------- SETORES COM TEMPOS DIFERENTES --------------------

      if(readKeypad == 'B'){

        // Já garanti que a memória é apagada previamente

        for(int i = 1; i <= 5 ; i++){
          endWhile = false;
          myTime = "";
          if(DEBUG) Serial.println("MyTime " + (String)i + " : " + (String)myTime);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Tempo setor " + (String)i + ":");

          while(true){
          char readKeypad = myKeypad.getKey(); // Faço a leitura do teclado
          if(readKeypad){
            switch(readKeypad){
              case 'A':
                if(myTime.length() > 0){
                  if(DEBUG) Serial.println(myTime);
                  // Salvo os dados do tempo na memória            
                  memorySaveTime(strToInt(myTime), flash, i, VALVE_TIME_SECTOR);
                  if(i == 5){
                    lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("Configurando");

                    for(int j = 0; j < 16 ; j++){
                      lcd.setCursor(j,1);
                      lcd.print(".");
                      delay(150);
                    }
                  }
                  endWhile = true;
                }

                break;

              case 'B': case 'C': case 'D': case '*': case '#':
                break;

              default:
                if(myTime.length() < 3){
                  myTime.concat(readKeypad);
                  lcd.setCursor(myTime.length()-1,1);
                  lcd.print(readKeypad);
                }
              }
            }
            if(endWhile) break;
          }
        }
      }
      break;
    }
  }
}

// ------------------------------------------------------------------------

// ============================================================================
// =============================== RTC FUNCTIONS ==============================
// ============================================================================

// Função que mantém o sistema na válvula pelo tempo configurado inicialmente
// Função deve buscar na memória o tempo ou tempo restante em que a válvula deve ficar ligada - e verificar a cada 1 min se o tempo foi atingido
// Caso o sistema seja desligado a função identifica o tempo restante para completar o ciclo e salva na memória esse valor...
// ... para que quando o sistema for ligado novamente esse valor seja utilizado
void keepTime(uint16_t _time){

  DateTime initialTime = rtc.now(); // Início da operação da válvula

// =============== DEBUGGING ===============
  if(DEBUG){
    Serial.print("Início: ");
    Serial.print(initialTime.year(), DEC);
    Serial.print('/');
    Serial.print(initialTime.month(), DEC);
    Serial.print('/');
    Serial.print(initialTime.day(), DEC);
    Serial.print(' ');
    Serial.print(initialTime.hour(), DEC);
    Serial.print(':');
    Serial.print(initialTime.minute(), DEC);
    Serial.print(':');
    Serial.print(initialTime.second(), DEC);
    Serial.println();
  }
// ==========================================

  DateTime future;
  if(_time >= 60){
    uint16_t hora = floor(_time/60);
    uint16_t minuto = _time - (hora*60);
    future = initialTime + TimeSpan(0, hora, minuto, 0); // Fim da operação
  }

  else{
    future = initialTime + TimeSpan(0, 0, _time, 0); // Fim da operação
  }
  
  // =============== DEBUGGING ===============
  if(DEBUG){
    Serial.print("Final: ");
    Serial.print(future.year(), DEC);
    Serial.print('/');
    Serial.print(future.month(), DEC);
    Serial.print('/');
    Serial.print(future.day(), DEC);
    Serial.print(' ');
    Serial.print(future.hour(), DEC);
    Serial.print(':');
    Serial.print(future.minute(), DEC);
    Serial.print(':');
    Serial.print(future.second(), DEC);
    Serial.println();
  }
  // ==========================================
  
  DateTime _now; // Recebe o tempo atual a cada verificação

  unsigned long previousMillis = millis(); // Tempo inicial de verificação
  uint16_t _interval = 60000; // Intervalo de verificação = 60 seg

  while(true){
    unsigned long currentMillis = millis();
    char readKeypad = myKeypad.getKey(); // Faço a leitura do teclado ====== LINHA UTILIZADA MERAMENTE PARA TESTE ======  _now = 22 ->> future = 5 [24 - _now + future]

    if (currentMillis - previousMillis >= _interval){
      previousMillis = currentMillis;
      _now = rtc.now(); // Verifico o tempo

      // Armazenando o tempo restante na memória
      short timeLeft;
      if(future.hour() - _now.hour() >= 0){
        timeLeft = ((60*future.hour()) + future.minute())-((60*_now.hour()) + _now.minute());
      }
      else{
        timeLeft = ((60*future.hour()) + future.minute())-((60*_now.hour()) + _now.minute()) + 1440;
      }

      flash.eraseSector(TIME_LEFT_SECTOR); // Limpa o espaço de memória onde está salvo o tempo restante
      
      // Salva na memória o tempo que resta para completar o ciclo
      if(timeLeft > 0) memorySaveTime(timeLeft, flash, valv, TIME_LEFT_SECTOR);
      else{
        // =============== DEBUGGING ===============
        if(DEBUG){
          Serial.println();
          Serial.println("Inicio: " + (String)initialTime.day() + "/" + (String)initialTime.month() + " | " + (String)initialTime.hour() + ":" + (String)initialTime.minute() + ":" + (String)initialTime.second());
          Serial.println("Agora: " + (String)_now.day() + "/" + (String)_now.month() + " | " + (String)_now.hour() + ":" + (String)_now.minute() + ":" + (String)_now.second());
          Serial.println("Final: " + (String)future.day() + "/" + (String)future.month() + " | " + (String)future.hour() + ":" + (String)future.minute() + ":" + (String)future.second());
          Serial.println("Tempo restante: " + (String)timeLeft);
          Serial.println();
    
          for(int i = 0; i<10; i++){
            Serial.println("Salvo na memória: " + (String)flash.readByte(TIME_LEFT_SECTOR+i));
          }
        }
        // ==========================================

        break; // Se o tempo é atingido então o programa segue para a próxima etapa
      }

      // =============== DEBUGGING ===============
      if(DEBUG){
        Serial.println();
        Serial.println("Inicio: " + (String)initialTime.day() + "/" + (String)initialTime.month() + " | " + (String)initialTime.hour() + ":" + (String)initialTime.minute() + ":" + (String)initialTime.second());
        Serial.println("Agora: " + (String)_now.day() + "/" + (String)_now.month() + " | " + (String)_now.hour() + ":" + (String)_now.minute() + ":" + (String)_now.second());
        Serial.println("Final: " + (String)future.day() + "/" + (String)future.month() + " | " + (String)future.hour() + ":" + (String)future.minute() + ":" + (String)initialTime.second());
        Serial.println("Tempo restante: " + (String)timeLeft);
        Serial.println();
  
        for(int i = 0; i<10; i++){
          Serial.println("Salvo na memória: " + (String)flash.readByte(TIME_LEFT_SECTOR+i));
        }
      }
      // ==========================================

    }
    if(!pression()){ // Não há pressão suiciente, então salvo o tempo restante
      //pression = !pression;
      _now = rtc.now();
      if(future.hour() - _now.hour() >= 0){
        timeLeft = ((60*future.hour()) + future.minute())-((60*_now.hour()) + _now.minute());
      }
      else{
        timeLeft = ((60*future.hour()) + future.minute())-((60*_now.hour()) + _now.minute()) + 1440;
      }

      flash.eraseSector(TIME_LEFT_SECTOR); // Limpa o espaço de memória onde está salvo o tempo restante
      if(timeLeft > 0){
        memorySaveTime(timeLeft, flash, valv, TIME_LEFT_SECTOR); // Salva na memória o tempo que resta para completar o ciclo
      }

      // =============== DEBUGGING ===============
      if(DEBUG){
        Serial.println("Agora: " + (String)_now.day() + "/" + (String)_now.month() + " | " + (String)_now.hour() + ":" + (String)_now.minute() + ":" + (String)_now.second());
        Serial.println("Futuro: " + (String)future.day() + "/" + (String)future.month() + " | " + (String)future.hour() + ":" + (String)future.minute() + ":" + (String)future.second());
        Serial.println("Tempo restante: " + (String)timeLeft);
        for(int i = 0; i<10; i++){
          Serial.println("Salvo na memória " + (String)(TIME_LEFT_SECTOR+i) + ": " + (String)flash.readByte(TIME_LEFT_SECTOR+i));
        }
      }
      // ==========================================
      if(timeLeft == 0){
        if(valv < 5)
          valv++;
        else
          valv = 1;
      }
      break;
    }
  }
}

// ------------------------------------------------------------------------
