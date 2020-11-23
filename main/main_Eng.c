

/*
    Autor: Prof. Luiz Paulo T.Juvencio.
    Objetivo:   Aula 14 - Integração dos Projetos Desenvolvidos
    Curso: Engenharia da Computação
*/
/*
Objetivo do trabalho: Desenvolver um projeto que integra diversos exemplos já replicados na disciplina.
Metodologia: Metodologia ativa onde o estudante desenvolve as habilidades conforme suas experiências passadas e com pesquisas.
Recursos: Será utilizado o AVA para documentação das atividades e o Teams como ferramenta de explicação e diálogo com os alunos.
Avaliação: Esta atividade será avaliativa peso 10,00. Acessar a aba de avaliações para mais informações.
*/
/*  Relação entre pinos da WeMos D1 R2 e GPIOs do ESP8266
    Pinos-WeMos     Função          Pino-ESP-8266
        TX          TXD             TXD/GPIO1
        RX          RXD             RXD/GPIO3
        D0          IO              GPIO16  
        D1          IO, SCL         GPIO5
        D2          IO, SDA         GPIO4
        D3          IO, 10k PU      GPIO0
        D4          IO, 10k PU, LED GPIO2
        D5          IO, SCK         GPIO14
        D6          IO, MISO        GPIO12
        D7          IO, MOSI        GPIO13
        D8          IO, 10k PD, SS  GPIO15
        A0          Inp. AN 3,3Vmax A0
*/

/* Bibliotecas */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "driver/gpio.h"
#include <ultrasonic.h>
#include <dht.h>
#include "esp_log.h"



/* Definições do hardware */
#define LED_STATUS 2
#define LED_2 13
#define BUTTON 0
static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
#define DHT11_GPIO 16          // DHT11
#define MAX_DISTANCE_CM 500 // 5m max
#define TRIGGER_GPIO 4      // Sensor ultrassonico 
#define ECHO_GPIO 5         // Sensor ultrassonico 

// Configurando o Wi-fi */
#define ESP_WIFI_SSID "SEU SSID"  // CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS "SUA SENHA" // CONFIG_ESP_WIFI_PASSWORD
#define ESP_MAXIMUM_RETRY 5       // CONFIG_ESP_MAXIMUM_RETRY
static int s_retry_num = 0;       // Quantidade de tentativas de conectar ao roteador
typedef enum led_status_wifi_t
{
    LED_STATUS_FAIL = 0,
    LED_STATUS_CONECTANDO,
    LED_STATUS_CONECTADO
} led_status_wifi_t;

// Configurando o Socket TCP *?
#define CONFIG_EXAMPLE_IPV4
#ifdef CONFIG_EXAMPLE_IPV4
#define HOST_IP_ADDR "192.168.137.184"
#else
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#endif

#define PORT 5005

/* Variáveis e definições globais */
QueueHandle_t tcp_queue;        // Fila de entrada de dados da task TCP
QueueHandle_t dht_queue;        // Fila de entrada de dados da task DHT
QueueHandle_t ultrasonic_queue; // Fila de entrada de dados da task Ultrasonic

static EventGroupHandle_t app_event_group;
#define WIFI_INIT_BIT BIT0
#define WIFI_CONNECTING_BIT BIT1
#define WIFI_CONNECTED_BIT BIT2
#define WIFI_IP_STA_GOT_IP BIT3
#define WIFI_FAIL_BIT BIT4


/* Como boa partica estes são os protótipos das funções */
static void task_ultrasonico(void *pvParamters);
static void dht_task(void *pvParameters);
static void wifi_init_sta(void);
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void task_GPIO_Control(void *pvParameter);
static void tcp_server_task(void *pvParameters);
static void task_button(void *pvParameter);

/* Aplicação */
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if( DEBUG )
            printf( "Tentando conectar ao WiFi...\r\n");
        
        xEventGroupClearBits(app_event_group, WIFI_FAIL_BIT);
        s_retry_num = 0;
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < ESP_MAXIMUM_RETRY) {
            
            esp_wifi_connect();
            s_retry_num++;
            printf( "Tentando reconectar ao WiFi...");
        } else {
            
            xEventGroupSetBits(app_event_group, WIFI_FAIL_BIT);
            xEventGroupClearBits(app_event_group, WIFI_CONNECTED_BIT); 
            printf("Falha ao conectar ao WiFi");
        }
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf( "Conectado! O IP atribuido é:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        
        xEventGroupSetBits(app_event_group, WIFI_CONNECTED_BIT);
    }
}

static void task_GPIO_Control(void *pvParameter)
{
    gpio_set_direction(LED_STATUS, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_STATUS, 0);

    led_status_wifi_t modo_led_status = LED_STATUS_FAIL;

    int tempo_led_status = 0;
    int timeout = 0;
    bool status = 0;

    while (1)
    {
        EventBits_t event_bits = xEventGroupGetBits(app_event_group);

        if (event_bits & WIFI_FAIL_BIT)
        {
            modo_led_status = LED_STATUS_FAIL;
            tempo_led_status = 2;
        }
        else if (event_bits & (WIFI_CONNECTED_BIT | WIFI_IP_STA_GOT_IP))
        {
            modo_led_status = LED_STATUS_CONECTADO;
        }
        else if (event_bits & WIFI_CONNECTING_BIT)
        {
            modo_led_status = LED_STATUS_CONECTANDO;
            tempo_led_status = 10;
        }
        else
        {
            modo_led_status = LED_STATUS_FAIL;
            tempo_led_status = 2;
        }

        if (modo_led_status == LED_STATUS_CONECTADO)
            gpio_set_level(LED_STATUS, 0); // 0 para ligar LED
        else
        {
            if (timeout >= tempo_led_status)
            {
                timeout = 0;
                status ^= 1;
                gpio_set_level(LED_STATUS, status);
            }
            timeout++;
        }

        vTaskDelay(50 / portTICK_PERIOD_MS); // tempo para piscar
    }
}

void task_button( void *pvParameter ){
    
    if( DEBUG )
      printf(  "Inicializada task_button...\r\n" ); 

    //gpio_set_direction( LED_2, GPIO_MODE_OUTPUT );
    //gpio_set_level( LED_BUILDING, 1 );  //O Led desliga em nível 1;  
    gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
    gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY );   
  
    //gpio_set_direction( LED_BUILDING, GPIO_MODE_OUTPUT );
    gpio_set_direction( LED_2, GPIO_MODE_OUTPUT );
    gpio_set_level(LED_2, 0);
    
    while (1) 
    {    
        /* o EventGroup bloqueia a task enquanto a rede WiFi não for configurada */
        xEventGroupWaitBits(app_event_group, WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);    
    
      // ADD codigo da tac:
      if (!gpio_get_level(BUTTON))
        {
            if( DEBUG )
                printf( "Botão Pressionado");         
            for (int i = 1; i < 7; i++){
                gpio_set_level(LED_2, i % 2); //pisca led
                vTaskDelay( 50 / portTICK_RATE_MS );
            }
            //conectar wifi
            xEventGroupClearBits(app_event_group, WIFI_FAIL_BIT);
            s_retry_num = 0;
            esp_wifi_connect();             
        }       
                
        vTaskDelay( 100 / portTICK_RATE_MS ); //Delay de 100ms liberando scheduler;
    }
      // Dentro pisca o led rapido(outro led) e coloco esp_conect e clear FAIL, limpar s_retry_num; 
    
}

static void tcp_server_task(void *pvParameters)
{
    printf( "tcp_server_task init");
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    static const char *msg = "Mensagem para ESP8266 \r\n";
    char payload[128];

    while (1)
    {
        xEventGroupWaitBits(app_event_group, (WIFI_CONNECTED_BIT | WIFI_IP_STA_GOT_IP), pdFALSE, pdFALSE, portMAX_DELAY);

#ifdef CONFIG_EXAMPLE_IPV4
        printf( "Conectado no IP:%s", HOST_IP_ADDR);
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 destAddr;
        inet6_aton(HOST_IP_ADDR, &destAddr.sin6_addr);
        destAddr.sin6_family = AF_INET6;
        destAddr.sin6_port = htons(PORT);
        destAddr.sin6_scope_id = tcpip_adapter_get_netif_index(TCPIP_ADAPTER_IF_STA);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0)
        {
            printf( "Unable to create socket: errno %d", errno);
            vTaskDelay(3000 / portTICK_PERIOD_MS); // Aguarda 3 segundos e tenta novamente
            continue;
        }

        printf( "Socket created");

        int err = connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0)
        {
            printf( "Socket unable to connect: errno %d", errno);
            vTaskDelay(3000 / portTICK_PERIOD_MS); // Aguarda 3 segundos e tenta novamente
            close(sock);
            continue;
        }
        printf( "Successfully connected");

        while (1)
        {
            // Envia os dados das filas ou a mensagem padrão.
            if (xQueueReceive(tcp_queue, &rx_buffer, pdMS_TO_TICKS(1000)) == true)
            {
                sprintf(payload, "%s", rx_buffer);
                printf( "-> payload: %s", payload);
            }
            else
            {
                sprintf(payload, "%s", msg);
                printf( "-> data: %s", msg);
            }

            int err = send(sock, payload, strlen(payload), 0);
            if (err < 0)
            {
                printf( "Error occured during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occured during receiving
            if (len < 0)
            {
                printf( "recv failed: errno %d", errno);
                break;
            }

            // Data received
            else
            {
                rx_buffer[len] = 0;
                printf( "Received %d bytes from %s:", len, addr_str);
                printf( "%s", rx_buffer);
                uint32_t req = 0;

                if (strstr((const char *)rx_buffer, "DHT") != NULL)
                {
                    req = 10;
                    xQueueSend(dht_queue, &req, 0); // 10 = Valor aleatório apenas para notificar a task dht para enviar dados
                }

                if (strstr((const char *)rx_buffer, "ULTRA") != NULL)
                {
                    req = 10;
                    xQueueSend(ultrasonic_queue, &req, 0); // 10 = Valor aleatório apenas para notificar a task ultrasonic para enviar dados
                }
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1)
        {
            printf( "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }

    vTaskDelete(NULL);
}

static void wifi_init_sta(void)
{
    app_event_group = xEventGroupCreate(); //Cria o grupo de eventos

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS
        },
    };

    /* Setting a password implies station will connect to all security modes including WEP/WPA.
        * However these modes are deprecated and not advisable to be used. Incase your Access point
        * doesn't support WPA2, these mode can be enabled by commenting below line */

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    printf( "wifi iniciou");
}

static void dht_task(void *pvParameters)
{
    printf( "Init");
    int16_t temperature = 0;
    int16_t humidity = 0;

    // DHT sensors that come mounted on a PCB generally have
    // pull-up resistors on the data pin.  It is recommended
    // to provide an external pull-up resistor otherwise...
    gpio_set_pull_mode(DHT11_GPIO, GPIO_PULLUP_ONLY);

    // Estabilizar o sensor
    vTaskDelay(50 / portTICK_PERIOD_MS);

    uint32_t req = 0;
    char res[128];

    while (1)
    {
        if (xQueueReceive(dht_queue, &req, pdMS_TO_TICKS(1000)) == true)
        {
            if (dht_read_data(sensor_type, DHT11_GPIO, &humidity, &temperature) == ESP_OK)
            {
                sprintf(res, "Humidade: %d%% Temperatura: %d°C\r\n", humidity / 10, temperature / 10);
                printf( "%s", res);
                xQueueSend(tcp_queue, &res, 0);
            }
            else
            {
                sprintf(res, "Could not read data from sensor dht\r\n");
                printf( "%s", res);
                xQueueSend(tcp_queue, &res, 0);
            }

            // 2000 porque deve estabilizar o componente, leituras mais rápidas 
            //esquentam o componente e erra a leitura de temperatura.
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }       
    }
}

static void task_ultrasonico(void *pvParamters)
{
    printf( "Init");
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO};

    ultrasonic_init(&sensor);

    // Estabilizar o sensor
    vTaskDelay(50 / portTICK_PERIOD_MS);
    uint32_t req = 0;
    char res[128];

    while (true)
    {
        if (xQueueReceive(ultrasonic_queue, &req, pdMS_TO_TICKS(1000)) == true)
        {            
            uint32_t distance;
            esp_err_t ret = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
            if (ret != ESP_OK)
            {   
                printf("Erro: ");
                switch (ret)
                {
                case ESP_ERR_ULTRASONIC_PING:
                    sprintf(res, "%s", "Cannot ping (device is in invalid state)\r\n");
                    printf( "%s", res);
                    xQueueSend(ultrasonic_queue, &res, 0); 
                    break;
                case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                    sprintf(res, "%s", "Ping timeout (no device found)\r\n");
                    printf( "%s", res);
                    xQueueSend(ultrasonic_queue, &res, 0); 
                    break;
                case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                    sprintf(res, "%s", "Echo timeout (i.e. distance too big)\r\n");
                    printf( "%s", res);
                    xQueueSend(ultrasonic_queue, &res, 0); 
                    break;
                default:
                    sprintf(res, "%u\r\n", ret);
                    printf( "%s", res);
                    xQueueSend(ultrasonic_queue, &res, 0);
                }
            }
            else
            {
                sprintf(res, "Distancia: %d cm\r\n", distance);
                printf( "%s", res);
                xQueueSend(tcp_queue, &res, 0);
            }
        }
        
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    printf( "Init app.");

    app_event_group = xEventGroupCreate();
    tcp_queue = xQueueCreate(10, sizeof(char) * 150);      // Cria *buffer* 
    dht_queue = xQueueCreate(10, sizeof(uint32_t));        // Cria *buffer* 
    ultrasonic_queue = xQueueCreate(10, sizeof(uint32_t)); // Cria *buffer* 

    // Serviços
    xTaskCreate(dht_task, "dht_task", configMINIMAL_STACK_SIZE * 2, NULL, 5, NULL);
    xTaskCreate(task_ultrasonico, "task_ultrasonico", configMINIMAL_STACK_SIZE * 2, NULL, 5, NULL);
    xTaskCreate(tcp_server_task, "tcp_server_task", configMINIMAL_STACK_SIZE * 2, NULL, 1, NULL);
    xTaskCreate(task_GPIO_Control, "task_GPIO_Control", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(task_button, "task_button", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    wifi_init_sta();
}
