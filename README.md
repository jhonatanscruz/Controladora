# Projeto_Rural
Projeto desenvolvido para a FTTECH sob demanda da TEZCA Válvulas.

O Objetivo é realizar o controle de válvulas através de um microcontrolador que envia comandos para um motor realizar a rotação do conjunto, que ainda conta com encoder. O Microcontrolador possui interação com uma memória Flash e com um RTC para a obtenção de um datalogger.

# Bibliotecas utilizadas:
- "FTTech_SAMD51Clicks.h" -> Funções de controle de energia da placa
- Keypad.h -> Funções para o teclado
- "FTTech_Components.h -> Funções de controle dos componentes do sistema, como Motor e Encoder
- Wire.h -> Necessário para interção I2C
- LiquidCrystal_I2C.h -> funções de controle da tela LCD além da exibição
- RTC_SAMD51.h -> Funções de controle do RTC
- DateTime.h -> Funções para manipulação de objetos tempo
- SPIMemory.h -> Funções para controle da memória Flash