#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/i2s_std.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RATE 16000
#define SAMPLES (RATE * 3)
static i2s_chan_handle_t rx;
static i2s_chan_handle_t tx;
static void prepare_rx(void);
static void prepare_tx(void);
#define AMP_ENABLE GPIO_NUM_18

static void send_text(const char *s) {
    uart_write_bytes(UART_NUM_0, s, strlen(s));
}

static void play_tone(void) {
    prepare_tx();
    // Both stereo slots carry the same quiet signal, independent of SD channel selection.
    ESP_ERROR_CHECK(i2s_channel_enable(tx));
    ESP_ERROR_CHECK(gpio_set_level(AMP_ENABLE, 1));
    int32_t stereo[128 * 2];
    for (int segment = 0; segment < 4; ++segment) {
        const int frames = (segment % 2 == 0) ? RATE / 4 : RATE / 8;
        const float frequency = segment == 0 ? 660.0f : 880.0f;
        for (int start = 0; start < frames; start += 128) {
            int count = frames - start;
            if (count > 128) count = 128;
            for (int i = 0; i < count; ++i) {
                const int n = start + i;
                float envelope = 1.0f;
                if (n < 160) envelope = n / 160.0f;
                if (frames - 1 - n < 160) envelope = (frames - 1 - n) / 160.0f;
                int16_t sample = (segment % 2 == 0)
                    ? (int16_t)(400.0f * envelope * sinf(6.28318530718f * frequency * n / RATE)) : 0;
                stereo[2 * i] = (int32_t)sample * 65536;
                stereo[2 * i + 1] = (int32_t)sample * 65536;
            }
            const size_t expected = count * 2 * sizeof(int32_t);
            size_t written = 0;
            ESP_ERROR_CHECK(i2s_channel_write(tx, stereo, expected, &written, 1000));
            ESP_ERROR_CHECK(written == expected ? ESP_OK : ESP_FAIL);
        }
    }
    // Let the trailing silence leave DMA before stopping the clocks.
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(gpio_set_level(AMP_ENABLE, 0));
    ESP_ERROR_CHECK(i2s_channel_disable(tx));
    send_text("TONE DONE\n");
}

static bool receive_audio(int16_t *pcm) {
    send_text("UPLOAD 96000\n");
    uint8_t *data = (uint8_t *)pcm;
    size_t offset = 0;
    uint32_t checksum = 0;
    while (offset < SAMPLES * sizeof(*pcm)) {
        size_t chunk = SAMPLES * sizeof(*pcm) - offset;
        if (chunk > 1024) chunk = 1024;
        send_text("NEXT\n");
        size_t received = 0;
        while (received < chunk) {
            int n = uart_read_bytes(UART_NUM_0, data + offset + received,
                                    chunk - received, pdMS_TO_TICKS(2000));
            if (n <= 0) {
                uart_flush_input(UART_NUM_0);
                send_text("ERROR upload timeout\n");
                return false;
            }
            received += n;
        }
        for (size_t i = 0; i < chunk; ++i) checksum += data[offset + i];
        offset += chunk;
    }
    char result[48];
    snprintf(result, sizeof(result), "RECEIVED %lu\n", (unsigned long)checksum);
    send_text(result);
    return true;
}

static void play_audio(const int16_t *pcm) {
    prepare_tx();
    send_text("PLAYING\n");
    ESP_ERROR_CHECK(i2s_channel_enable(tx));
    ESP_ERROR_CHECK(gpio_set_level(AMP_ENABLE, 1));
    int32_t stereo[128 * 2];
    // Send the clip, followed by enough silence to drain the DMA queue.
    for (int start = 0; start < SAMPLES + RATE / 8; start += 128) {
        int count = SAMPLES + RATE / 8 - start;
        if (count > 128) count = 128;
        for (int i = 0; i < count; ++i) {
            int16_t sample = start + i < SAMPLES ? pcm[start + i] : 0;
            stereo[2 * i] = (int32_t)sample * 65536;
            stereo[2 * i + 1] = (int32_t)sample * 65536;
        }
        size_t expected = count * 2 * sizeof(int32_t), written = 0;
        ESP_ERROR_CHECK(i2s_channel_write(tx, stereo, expected, &written, 1000));
        ESP_ERROR_CHECK(written == expected ? ESP_OK : ESP_FAIL);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(gpio_set_level(AMP_ENABLE, 0));
    ESP_ERROR_CHECK(i2s_channel_disable(tx));
    send_text("PLAY DONE\n");
}

static void prepare_rx(void) {
    if (rx) return;
    if (tx) { ESP_ERROR_CHECK(i2s_del_channel(tx)); tx = NULL; }
    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&channel, NULL, &rx));
    i2s_std_config_t cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED, .bclk = GPIO_NUM_4, .ws = GPIO_NUM_5,
            .dout = I2S_GPIO_UNUSED, .din = GPIO_NUM_6,
            .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
        },
    };
    cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx, &cfg));
}

static void prepare_tx(void) {
    if (tx) return;
    if (rx) { ESP_ERROR_CHECK(i2s_del_channel(rx)); rx = NULL; }
    i2s_chan_config_t output_channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    output_channel.dma_desc_num = 4;
    output_channel.dma_frame_num = 256;
    output_channel.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&output_channel, &tx, NULL));
    i2s_std_config_t output_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED, .bclk = GPIO_NUM_15, .ws = GPIO_NUM_16,
            .dout = GPIO_NUM_17, .din = I2S_GPIO_UNUSED,
            .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx, &output_cfg));
}

void app_main(void) {
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 4096, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_set_baudrate(UART_NUM_0, 921600));
    ESP_ERROR_CHECK(gpio_reset_pin(AMP_ENABLE));
    ESP_ERROR_CHECK(gpio_set_level(AMP_ENABLE, 0));
    ESP_ERROR_CHECK(gpio_set_direction(AMP_ENABLE, GPIO_MODE_OUTPUT));
    prepare_tx();
    // Clocks and acquisition run only during an explicitly requested recording.
    int16_t *pcm = malloc(SAMPLES * sizeof(*pcm));
    if (!pcm) { send_text("ERROR memory\n"); return; }
    bool playback_valid = false;
    send_text("READY audio-test v5; I2S0 TX32; SD=GPIO18; r=record, t=tone, p=upload, g=play\n");
    for (;;) {
        uint8_t command;
        if (uart_read_bytes(UART_NUM_0, &command, 1, pdMS_TO_TICKS(100)) != 1) continue;
        if (command == 't') { play_tone(); continue; }
        if (command == 'p') { playback_valid = receive_audio(pcm); continue; }
        if (command == 'g') {
            if (playback_valid) play_audio(pcm);
            else send_text("ERROR no uploaded audio\n");
            continue;
        }
        if (command != 'r') continue;
        playback_valid = false;
        prepare_rx();
        ESP_ERROR_CHECK(i2s_channel_enable(rx));
        int32_t block[256];
        size_t bytes;
        // Discard startup transient.
        for (int i = 0; i < 4; ++i)
            ESP_ERROR_CHECK(i2s_channel_read(rx, block, sizeof(block), &bytes, 1000));
        send_text("RECORDING\n");
        size_t count = 0;
        bool failed = false;
        while (count < SAMPLES) {
            esp_err_t err = i2s_channel_read(rx, block, sizeof(block), &bytes, 1000);
            if (err != ESP_OK || bytes == 0 || bytes % sizeof(int32_t)) { failed = true; break; }
            for (size_t i = 0; i < bytes / sizeof(int32_t) && count < SAMPLES; ++i)
                pcm[count++] = (int16_t)(block[i] >> 16);
        }
        ESP_ERROR_CHECK(i2s_channel_disable(rx));
        if (failed) { send_text("ERROR capture\n"); continue; }
        send_text("AUDIO 96000\n");
        uart_write_bytes(UART_NUM_0, pcm, SAMPLES * sizeof(*pcm));
        send_text("\nDONE\n");
    }
}
