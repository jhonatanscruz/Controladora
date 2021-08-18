//---------------------------------------------------------------------------------
// Transforma um dígito inteiro declado como CHAR para o tipo INT
int charToInt(char c){
  int base = (int)'0';
  int num = (int)c;
  return (c - base);
}

// ------------------------------------------------------------------------
// Transforma um número inteiro declado como STRING para o tipo INT
int strToInt(String str){

  int numb = 0;
  for(int i = 0; i < str.length() ; i++){
    numb = numb + (charToInt(str[i]) * pot(10,str.length()-(i+1)));
  }
  return numb;
}

// ------------------------------------------------------------------------
// Faz a operação de potência dados a base e o expoente
int pot(int base, int expo){
  int result = 1;
  for(int i = 0 ; i < expo ; i++){
      result = result * base;
  }
  return result;
}

//---------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------

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
  Serial.begin(9600); // Inicia porta serial
  while(!Serial);

  // AttachInterrupt0, digital PinA, Signal A
  // It Activates interruption on any signal change
  attachInterrupt(digitalPinToInterrupt(pinA), PinA_OnChange, CHANGE);

  // AttachInterrupt1, digital PinB, Signal B
  // It Activates interruption on any signal change
  attachInterrupt(digitalPinToInterrupt(pinB), PinB_OnChange, CHANGE);

  //AttachInterrupt2, digital PinZ, Signal Z
  // It Activates interruption at the falling edge of the signal
  attachInterrupt(digitalPinToInterrupt(pinZ), PinZ_OnFalling, FALLING);
  
  Serial.println("Iniciou!");
  
  initAnimation();
}

//---------------------------------------------------------------------------------
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

//---------------------------------------------------------------------------------

void systemSetup(){

// Set system to zero
  if(!onZero){
    motor.changeDIR(1);
    motor.start();
    while(!onZero){
      motor.pulse();
    }

    encoder.reset();
  
    // Offset to adjust the motor position to zero point
    motor.start();
    while(encoder.getPosition() > ENCODER_POSITION[1]){
      motor.pulse();
    }
      motor.stop();

    // Now system is in zero point so its parameters should be reseted
    motor.reset();
    motor.changeDIR(1); // Motor rotates clockwise 
    encoder.reset();
  
    if(flash.readByte(4096) == 255){ // Verifico se já existe uma válvula salva previamente
      flash.writeByte(4096, valv); // Salvo na memória a primeira válvula a ser ligada
    }
  }
}

//---------------------------------------------------------------------------------

void printMenu(){
  switch(screen){
    case 1:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Sistema Ligado!!");
      lcd.setCursor(3,1);
      lcd.print("Valvula: ");
      lcd.setCursor(12,1);
      lcd.print(valv);
      break;

    case 2:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Bomba desligada!");
  }
}

//---------------------------------------------------------------------------------

uint8_t getBatteryPercentual(){
  byte minValue = 5;
  byte maxValue = 8;
  return (FTClicks.readLeadAcidBattery() - minValue) * 100 / (maxValue-minValue);
}

//---------------------------------------------------------------------------------

void screenAction(char readKeypad){

  switch(screen){
    case 1:
      switch(readKeypad){
        case '*':
          if(!pression) printPreviousScreen();
          break;

        case '#':
          if(!pression) printNextScreen();
          break;
      }
      break;

    case 2:
      switch(readKeypad){
        case '*':
          printPreviousScreen();
          break;
          
        case '#':
          printNextScreen();
          break;
      }
      break;
  }
}

void printNextScreen(){
  (screen == 2) ? screen = 1 : screen++;
  printMenu();  
}

void printPreviousScreen(){
  (screen == 1) ? screen = 2 : screen--;
  printMenu();
}

// ============================================================================
// ============================ SPI MEMORY FUNCTIONS ==========================
// ============================================================================

// Salva na memória o tempo de abertura de cada válvula
void memorySaveTime(uint16_t timing, SPIFlash flash, int valv, int pos){

    // Se o tempo for menor ou igual a 254 => Consigo salvar em 1 Byte
  if(timing <= 254){
    for(int i = 0; i < 9; i = i+2){
      if(i == 2*(valv-1))
      {
        // Escreve na memória
        flash.writeByte(i+pos, 0); // Primeiro Byte recebe 0
        flash.writeByte(i+1+pos, timing); // Segundo Byte recebe o tempo em minutos
      }
    }
  }

  // Se o tempo for superior a 254 => Divido o valor em 2 Bytes
  else{
    for(int i = 0; i < 9; i = i+2)
    {
      if(i == 2*(valv-1))
      {
        // Operação para separar o valor em 2 Bytes
        int byte1 = (int)floor(timing/10); // Primeiro Byte = Parte inteira da divisão por 10 do valor inicial
        int byte2 = timing - (10*byte1); // Segundo Byte = Parte unitária do valor inicial

        // Escreve na memória
        flash.writeByte(i+pos, byte1);
        flash.writeByte(i+1+pos, byte2);
      }
    }
  }
}

// ------------------------------------------------------------------------

// Recupera os valores salvos anteriormente na memória
uint16_t memoryGetTime(SPIFlash flash, int valv, int pos){

  uint16_t timing;
  uint16_t pos1;
  uint16_t pos2;

  // Procuro o tempo salvo para a válvula passada no parâmetro
  for(int i = 0; i < 9; i = i+2){
    if(i == 2*(valv-1)){
      pos1 = flash.readByte(i+pos);
      pos2 = flash.readByte(i+1+pos);
    }
  }

  // Se o tempo foi salvo em apenas 1 Byte
  if(pos1 == 0){
    return pos2;
  }
  
  // Caso o tempo tenha sido salvo em 2 Bytes
  else{
    timing  = (pos1*10)+pos2;
    return timing;
  }
}

// ------------------------------------------------------------------------

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
                    memorySaveTime(strToInt(myTime), flash, i, 0);
                    Serial.println("Salvei "+(String)strToInt(myTime)+ " no endereço " + (String)i);
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
          Serial.println("MyTime " + (String)i + " : " + (String)myTime);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Tempo setor " + (String)i + ":");

          while(true){
          char readKeypad = myKeypad.getKey(); // Faço a leitura do teclado
          if(readKeypad){
            switch(readKeypad){
              case 'A':
                if(myTime.length() > 0){
                  Serial.println(myTime);
                  // Salvo os dados do tempo na memória            
                  memorySaveTime(strToInt(myTime), flash, i, 0);
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

void keepTime(uint16_t _time){

  DateTime initialTime = rtc.now(); // Início da operação da válvula
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

  DateTime future;
  if(_time >= 60){
    uint16_t hora = floor(_time/60);
    uint16_t minuto = _time - (hora*60);
    future = initialTime + TimeSpan(0, hora, minuto, 0); // Fim da operação
  }

  else{
    future = initialTime + TimeSpan(0, 0, _time, 0); // Fim da operação
  }

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
      uint16_t timeLeft;
      if(future.hour() - _now.hour() >= 0){
        timeLeft = ((60*future.hour()) + future.minute())-((60*_now.hour()) + _now.minute());
      }
      else{
        timeLeft = ((60*future.hour()) + future.minute())-((60*_now.hour()) + _now.minute()) + 1440;
      }

      flash.eraseSector(8192); // Limpa o espaço de memória onde está salvo o tempo restante
      memorySaveTime(timeLeft, flash, valv, 8192); // Salva na memória o tempo que resta para completar o ciclo

      // =============== DEBUGGING ===============
      Serial.println();
      Serial.println("Inicio: " + (String)initialTime.day() + "/" + (String)initialTime.month() + " | " + (String)initialTime.hour() + ":" + (String)initialTime.minute() + ":" + (String)initialTime.second());
      Serial.println("Agora: " + (String)_now.day() + "/" + (String)_now.month() + " | " + (String)_now.hour() + ":" + (String)_now.minute() + ":" + (String)_now.second());
      Serial.println("Final: " + (String)future.day() + "/" + (String)future.month() + " | " + (String)future.hour() + ":" + (String)future.minute() + ":" + (String)initialTime.second());
      Serial.println("Tempo restante: " + (String)timeLeft);
      Serial.println();

      for(int i = 0; i<10; i++){
        Serial.println("Salvo na memória: " + (String)flash.readByte(8192+i));
      }
     // ==========================================

      if(_now >= future){
        flash.eraseSector(8192); // Tempo restante = 0
        break; // Se o tempo é atingido então o programa segue para a próxima etapa
      }
    }

    if(readKeypad == 'D'){ // ========== UTILIZAR FUTURAMENTE LEITURA DO SENSOR DE PRESSÃO ==========
      pression = !pression;
      _now = rtc.now();
      timeLeft = future.minute() - _now.minute(); // Armazeno o tempo que resta para completar o ciclo
      Serial.println("Minuto Futuro: " + (String)future.minute());
      Serial.println("Minuto Agora: " + (String)_now.minute());
      Serial.println("Tempo restante: " + (String)timeLeft);
      if(timeLeft == 0){
        if(valv < 5)
          valv++;
        else
          valv = 1;
      }
    }

    if(!pression) break; // Se Não há pressão suficiente então quebro o while

  }
}

// ------------------------------------------------------------------------

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

// ------------------------------------------------------------------------

bool needConfig(SPIFlash flash){
  if(flash.readByte(0) != 255){
    return false;
  }

  else{
    return true;
  }
}

// ------------------------------------------------------------------------

void resetSystem(){

  //Apago os blocos de memória
  flash.eraseSector(0); // Tempos de válvulas
  flash.eraseSector(4096); // número da válvula atual
  flash.eraseSector(8192); // Tempo restante na válvula atual
  
  valv = 1;
  onZero = false;
  Serial.println("Iniciou!");
  initAnimation();
  systemSetup();
  setTime();
}
