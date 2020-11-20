# ESP8266_Socket_TCP-Wi-Fi

## Materiais utilizados.

1. Wemos D1 com cabo usb micro
<img src="https://github.com/LuizPauloDesigner/ESP8266_Socket_TCP-Wi-Fi/blob/main/img/4-4.png" height="250" width="400" >

2. Sensor de temperatura DHT11
<img src="https://github.com/LuizPauloDesigner/ESP8266_Socket_TCP-Wi-Fi/blob/main/img/Dht11.jpg" height="248" width="400" >

3. Sensor ultrassonico HC-SR04
<img src="https://github.com/LuizPauloDesigner/ESP8266_Socket_TCP-Wi-Fi/blob/main/img/15569-Ultrasonic_Distance_Sensor_-_HC-SR04-01a.jpg" height="248" width="400" >

4. Tacswitch
<img src="https://github.com/LuizPauloDesigner/ESP8266_Socket_TCP-Wi-Fi/blob/main/img/ProdutoDestaque_11193_orig.jpg" height="150" width="150" >

5. Jumpers coloridos macho/macho
<img src="https://github.com/LuizPauloDesigner/ESP8266_Socket_TCP-Wi-Fi/blob/main/img/cables-jumper-para-protoboar-macho-macho-raspberry-robotica-892201-MLM8464485218_052015-F.png" height="250" width="250" >

6. Protoboard 830 pontos
<img src="https://github.com/LuizPauloDesigner/ESP8266_Socket_TCP-Wi-Fi/blob/main/img/protoboard-830-pontos-ee05c6b3.jpg" height="200" width="400" > 

## Ferramentas usadas para compilação de codigo.

-Usamos o ambiente > https://www.msys2.org/.
-Conjunto de ferramentas para o ESP8266.
O passo a passo para a instalação pode ser encontrada no link abaixo.
> https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/

## Como usar este Projeto

* Copie os arquivos da pasta components disponizados neste repositorio e cole na seguinte pasta:
"ESP8266_RTOS_SDK/components"

* Localize o arquivo main_Eng.c na pasta main disponibilizado no repositorio.

Você devera configurar as seguintes linhas do arquivo.
*Configure seu SSID e sua senha de sua rede WI-FI
  ```
  // Configurando o Wi-fi */
  #define ESP_WIFI_SSID "SEU SSID"  // CONFIG_ESP_WIFI_SSID
  #define ESP_WIFI_PASS "SUA SENHA" // CONFIG_ESP_WIFI_PASSWORD
  ```
  *Configure o IP e a Porta do servidor
  ```
  // Configurando o Socket TCP *?
  #define CONFIG_EXAMPLE_IPV4
  #ifdef CONFIG_EXAMPLE_IPV4
  #define HOST_IP_ADDR "192.168.137.184"
  #else
  #define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
  #endif
  
  #define PORT 5005
  ```



