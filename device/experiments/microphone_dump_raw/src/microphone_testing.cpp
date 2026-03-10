#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_WS  2
#define I2S_SD  1
#define I2S_SCK 42

#define I2S_PORT I2S_NUM_0
#define BUFFER_SAMPLES 256

int32_t samples[BUFFER_SAMPLES];

void setup() {
  Serial.begin(115200);
  delay(1000);

  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);

  Serial.println("Mic level test ready");
}

void loop() {
  size_t bytesRead = 0;
  i2s_read(I2S_PORT, samples, sizeof(samples), &bytesRead, portMAX_DELAY);

  int count = bytesRead / sizeof(int32_t);
  long long sumAbs = 0;

  for (int i = 0; i < count; i++) {
    int32_t s = samples[i] >> 14;   // scale down INMP441 24-bit data in 32-bit frame
    sumAbs += llabs((long long)s);
  }

  long level = sumAbs / count;
  Serial.println(level);

  delay(50);
}