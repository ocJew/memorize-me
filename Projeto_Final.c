#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "ws2812b.pio.h"
#include "hardware/adc.h"      // Biblioteca para controle do ADC (Conversor Analógico-Digital).
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
//#include "ssd1306.h"

const uint BUTTON_A = 5; 
const uint LED_VERDE = 11; 
const uint LED_VERMELHO = 13; 

#define I2C_SDA 14
#define I2C_SCL 15
#define BUZZER_PIN 21  // Define o pino do buzzer (pino 21)
#define VRX_PIN 26    // Define o pino GP26 para o eixo X do joystick (Canal ADC0).
#define VRY_PIN 27    // Define o pino GP27 para o eixo Y do joystick (Canal ADC1).
#define SW_PIN 22     // Define o pino GP22 para o botão do joystick (entrada digital).
#define CENTER 2048      // Valor central do ADC (para 12 bits, intervalo 0-4095)
#define DEADZONE 200     // Faixa de tolerância para movimentos leves
#define MAX_ROUNDS 100  // Limite máximo de rodadas para evitar loops infinitos
#define LED_PIN 7       // GPIO conectado ao Data In do primeiro LED
#define NUM_LEDS 25     // Número de LEDs na linha

int sequencia[MAX_ROUNDS];  // Array para armazenar a sequência gerada
int rodada = 0;             // Contador de rodadas
int acertou = 1;            // Flag para verificar se o usuário acertou

uint32_t led_colors[NUM_LEDS];  // Buffer para armazenar cores de cada LED

void init_i2c() {
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void ws2812_init(PIO pio, int sm, uint pin) {
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, pin, 800000, false);
}

// Função para enviar o buffer de cores para os LEDs
void update_leds(PIO pio, int sm) {
    for (int i = 0; i < NUM_LEDS; i++) {
        pio_sm_put_blocking(pio, sm, led_colors[i] << 8);
    }
}

// Converte RGB para GRB (formato usado pelo WS2812)
uint32_t rgb_to_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((g << 16) | (r << 8) | b);
}

void seta_direita() {
    led_colors[2] = rgb_to_grb(0, 64, 0);
    led_colors[6] = rgb_to_grb(0, 64, 0);
    led_colors[10] = rgb_to_grb(0, 64, 0);
    led_colors[11] = rgb_to_grb(0, 64, 0);
    led_colors[12] = rgb_to_grb(0, 64, 0);
    led_colors[13] = rgb_to_grb(0, 64, 0);
    led_colors[14] = rgb_to_grb(0, 64, 0);
    led_colors[16] = rgb_to_grb(0, 64, 0);
    led_colors[22] = rgb_to_grb(0, 64, 0);
}

void seta_esquerda() {
    led_colors[2] = rgb_to_grb(0, 64, 0);
    led_colors[8] = rgb_to_grb(0, 64, 0);
    led_colors[10] = rgb_to_grb(0, 64, 0);
    led_colors[11] = rgb_to_grb(0, 64, 0);
    led_colors[12] = rgb_to_grb(0, 64, 0);
    led_colors[13] = rgb_to_grb(0, 64, 0);
    led_colors[14] = rgb_to_grb(0, 64, 0);
    led_colors[18] = rgb_to_grb(0, 64, 0);
    led_colors[22] = rgb_to_grb(0, 64, 0);
}

void seta_cima() {
    led_colors[2] = rgb_to_grb(0, 64, 0);
    led_colors[6] = rgb_to_grb(0, 64, 0);
    led_colors[7] = rgb_to_grb(0, 64, 0);
    led_colors[8] = rgb_to_grb(0, 64, 0);
    led_colors[10] = rgb_to_grb(0, 64, 0);
    led_colors[12] = rgb_to_grb(0, 64, 0);
    led_colors[14] = rgb_to_grb(0, 64, 0);
    led_colors[17] = rgb_to_grb(0, 64, 0);
    led_colors[22] = rgb_to_grb(0, 64, 0);
}

void seta_baixo() {
    led_colors[2] = rgb_to_grb(0, 64, 0);
    led_colors[7] = rgb_to_grb(0, 64, 0);
    led_colors[10] = rgb_to_grb(0, 64, 0);
    led_colors[12] = rgb_to_grb(0, 64, 0);
    led_colors[14] = rgb_to_grb(0, 64, 0);
    led_colors[16] = rgb_to_grb(0, 64, 0);
    led_colors[17] = rgb_to_grb(0, 64, 0);
    led_colors[18] = rgb_to_grb(0, 64, 0);
    led_colors[22] = rgb_to_grb(0, 64, 0);
}

void escolha() {
    int escolha = rand() % 4;
    rodada++;
    sequencia[rodada - 1] = escolha; // Adiciona um novo número à sequência
}

void mostrar_seta(int numero) {
    if (numero == 0) {
        seta_direita();
    } else if (numero == 1) {
        seta_esquerda();
    } else if (numero == 2) {
        seta_cima();
    } else if (numero == 3) {
        seta_baixo();
    }
}

void apagar_leds() {
    // Apaga todos os LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
        led_colors[i] = rgb_to_grb(0, 0, 0);
    }
}

void tocar_buzzer(uint16_t frequencia, uint32_t duracao_ms) {
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    // Emite um som alternando o estado do pino (dentro da frequência desejada)
    uint32_t tempo_invertido = 1000000 / frequencia;  // Calcula o tempo de inversão de estado
    uint32_t tempo_fim = time_us_32() + duracao_ms * 1000;
    while (time_us_32() < tempo_fim) {
        gpio_put(BUZZER_PIN, 1);  // Define o pino como HIGH
        sleep_us(tempo_invertido); // Aguarda meio ciclo
        gpio_put(BUZZER_PIN, 0);  // Define o pino como LOW
        sleep_us(tempo_invertido); // Aguarda meio ciclo
    }
}

void som_click() {
    // Emite um som rápido de click (1.000 Hz) por 100 ms
    tocar_buzzer(1000, 100);  // Som de click de 1.000 Hz por 100 ms
}

void som_click2() {
    tocar_buzzer(700, 80);  // Som de click de 500 Hz por 100 ms
}

void som_vitoria() {
    gpio_put(LED_VERDE, true);         
    tocar_buzzer(400, 500);  // Som de vitória de 400 Hz por 500 ms
    gpio_put(LED_VERDE, false); 
}

void som_derrota() {
    gpio_put(LED_VERMELHO, true); 
    tocar_buzzer(200, 500);  // Som de derrota de 200 Hz por 500 ms
    gpio_put(LED_VERMELHO, false); 
}

int main() {

    stdio_init_all();
    init_i2c();
    ssd1306_init();
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);
    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);
    int resposta = -1;

    gpio_init(BUZZER_PIN);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A); 

    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0); 

    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_put(LED_VERMELHO, 0); 

    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    
    bool button_pressed = !gpio_get(BUTTON_A); 
    printf("Bem-vindo ao jogo da memória!\n");
    printf("Lembre-se da sequência e digite os números corretamente.\n");
    printf("Aperte A para iniciar.\n\n");

    memset(ssd, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd, 5, 0, "  Aperte A  ");
    render_on_display(ssd, &frame_area);
    ssd1306_draw_string(ssd, 5, 20, "  para iniciar  ");
    render_on_display(ssd, &frame_area);
    
    while (gpio_get(BUTTON_A) != 0) {  // Espera até o botão ser pressionado (valor lógico baixo)
        sleep_ms(100);  // Aguarda 100ms para evitar checagens rápidas demais
    }
    som_click();
    printf("Prepara-se...\n\n");

    memset(ssd, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd, 5, 0, "Prepara-se");
    render_on_display(ssd, &frame_area);

    srand(time(NULL));
    stdio_init_all();  // Inicializa comunicação UART
    PIO pio = pio0;
    int sm = 0;  // State machine 0
    ws2812_init(pio, sm, LED_PIN);
    // Inicializa todos os LEDs apagados
    apagar_leds();
    
    while (true) {
        if (acertou && rodada < MAX_ROUNDS) {

            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 5, 0, "  Memorize-me  ");
            render_on_display(ssd, &frame_area);

            sleep_ms(1000);
            escolha(); // Adiciona um novo número à sequência
            // Mostra a sequência para o jogador memorizar
            for (int i = 0; i < rodada; i++) {
                mostrar_seta(sequencia[i]);
                som_click2();
                update_leds(pio, sm);  // Envia o buffer para os LEDs
                sleep_ms(500);
                apagar_leds();
                update_leds(pio, sm);
                sleep_ms(500);
            }
            printf("\n");

            // O jogador tenta repetir a sequência
            printf("Digite a sequência completa:\n");

            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 5, 0, "Use o joystick");
            render_on_display(ssd, &frame_area);
            
            sleep_ms(100);

            for (int i = 0; i < rodada; i++) {
                char buffer[10];  // Para armazenar o número convertido em string
                sprintf(buffer, "      %d", i+1);  // Converte a variável 'i' em uma string
                ssd1306_draw_string(ssd, 5, 20, buffer); 
                render_on_display(ssd, &frame_area);

                int resposta = -1;
                printf("Número #%d: ", i + 1);
                
                // Aguarda até que o jogador forneça uma entrada válida
                while (resposta == -1) {
                    adc_select_input(0);  // Seleciona o pino X
                    uint16_t vrx_value = adc_read();
                    adc_select_input(1);  // Seleciona o pino Y
                    uint16_t vry_value = adc_read();

                    // Lógica para determinar a direção com base no joystick
                    if (vry_value < CENTER - DEADZONE) {
                        resposta = 0;  // Direita
                    } else if (vry_value > CENTER + DEADZONE) {
                        resposta = 1;  // Esquerda
                    } else if (vrx_value < CENTER - DEADZONE) {
                        resposta = 2;  // Cima
                    } else if (vrx_value > CENTER + DEADZONE) {
                        resposta = 3;  // Baixo
                    }

                    sleep_ms(200);  // Aguarda um tempo antes de verificar novamente
                }

                // Lê o estado do botão (SW)
                bool sw_value = gpio_get(SW_PIN) == 0;

                // Verifica se a resposta está correta
                if (resposta != sequencia[i]) {
                    memset(ssd, 0, ssd1306_buffer_length);
                    ssd1306_draw_string(ssd, 5, 0, "  Errou  ");
                    render_on_display(ssd, &frame_area);
                    
                    printf("Errado! Você perdeu na rodada %d.\n", rodada);
                    som_derrota();
                    acertou = 0;
                    rodada = 0;
                    break;
                }
                som_click();
            }

            if (acertou) {
                char buffer[32];  // Buffer para armazenar a string formatada
                memset(ssd, 0, ssd1306_buffer_length);
                sprintf(buffer, "     Rodada %d.", rodada);
                ssd1306_draw_string(ssd, 5, 0, buffer); 
                render_on_display(ssd, &frame_area);

                printf("Parabéns! Você completou a rodada %d.\n\n", rodada);
                som_vitoria();
                printf("Prepare-se para a rodada %d.\n\n", rodada + 1);
            }
        }

        if (rodada == MAX_ROUNDS) {
            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 5, 0, "GAME OVER");
            render_on_display(ssd, &frame_area);

            printf("Você completou o jogo! Parabéns!\n");
            printf("Obrigado por jogar!\n");
            break;  // Encerra o loop do jogo
        }

        if (rodada == 0) {
            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 5, 0, "  Aperte A  ");
            render_on_display(ssd, &frame_area);

            ssd1306_draw_string(ssd, 5, 20, " para reiniciar");
            render_on_display(ssd, &frame_area);

            printf("Aperte A para Reiniciar.\n\n");
            while (gpio_get(BUTTON_A) != 0) {  // Espera até o botão ser pressionado (valor lógico baixo)
                sleep_ms(100);  // Aguarda 100ms para evitar checagens rápidas demais
            }
            som_click();
            printf("Prepara-se...\n\n");
            sleep_ms(500);
            acertou = 1;
        }
    }

    return 0;
}
    
