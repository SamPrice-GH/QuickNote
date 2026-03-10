#include <Arduino.h>
#include <driver/i2s.h>
#include "FS.h"
#include "SD_MMC.h"

#define I2S_PORT I2S_NUM_0

// Your current wiring
#define I2S_SD   1
#define I2S_WS   2
#define I2S_SCK  42

#define SD_MMC_CMD 38 //Please do not modify it.
#define SD_MMC_CLK 39 //Please do not modify it. 
#define SD_MMC_D0  40 //Please do not modify it.

#define SAMPLE_RATE     16000
#define BITS_PER_SAMPLE 16
#define RECORD_SECONDS  5
#define WAV_FILE_NAME   "/test.wav"

// Simple WAV header for PCM
void writeWavHeader(File &file, uint32_t dataSize) {
  uint32_t fileSize = dataSize + 36;
  uint16_t audioFormat = 1;   // PCM
  uint16_t numChannels = 1;   // mono
  uint32_t sampleRate = SAMPLE_RATE;
  uint16_t bitsPerSample = BITS_PER_SAMPLE;
  uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
  uint16_t blockAlign = numChannels * (bitsPerSample / 8);
  uint32_t subchunk1Size = 16;

  file.seek(0);

  file.write((const uint8_t *)"RIFF", 4);
  file.write((uint8_t *)&fileSize, 4);
  file.write((const uint8_t *)"WAVE", 4);

  file.write((const uint8_t *)"fmt ", 4);
  file.write((uint8_t *)&subchunk1Size, 4);
  file.write((uint8_t *)&audioFormat, 2);
  file.write((uint8_t *)&numChannels, 2);
  file.write((uint8_t *)&sampleRate, 4);
  file.write((uint8_t *)&byteRate, 4);
  file.write((uint8_t *)&blockAlign, 2);
  file.write((uint8_t *)&bitsPerSample, 2);

  file.write((const uint8_t *)"data", 4);
  file.write((uint8_t *)&dataSize, 4);
}

void setupI2S() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
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
  i2s_zero_dma_buffer(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Mounting SD card...");
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    Serial.println("Card Mount Failed");
    return;
  }

  Serial.println("Initialising I2S...");
  setupI2S();

  File file = SD_MMC.open(WAV_FILE_NAME, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open WAV file");
    while (true) delay(1000);
  }

  // Reserve space for WAV header
  uint8_t emptyHeader[44] = {0};
  file.write(emptyHeader, 44);

  const size_t samplesPerRead = 256;
  int32_t i2sBuffer[samplesPerRead];
  int16_t pcmBuffer[samplesPerRead];

  uint32_t totalBytesWritten = 0;
  uint32_t totalReads = (SAMPLE_RATE * RECORD_SECONDS) / samplesPerRead;

  Serial.println("Recording starts in 2 seconds...");
  delay(2000);
  Serial.println("Recording now");

  for (uint32_t r = 0; r < totalReads; r++) {
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, i2sBuffer, sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);

    int count = bytesRead / sizeof(int32_t);

    for (int i = 0; i < count; i++) {
      // INMP441 gives 24-bit data in a 32-bit frame.
      // Shift down and clamp to 16-bit PCM.
      int32_t sample = i2sBuffer[i] >> 14;

      if (sample > 32767) sample = 32767;
      if (sample < -32768) sample = -32768;

      pcmBuffer[i] = (int16_t)sample;
    }

    size_t bytesToWrite = count * sizeof(int16_t);
    file.write((uint8_t *)pcmBuffer, bytesToWrite);
    totalBytesWritten += bytesToWrite;
  }

  writeWavHeader(file, totalBytesWritten);
  file.close();

  Serial.println("Recording complete");
  Serial.print("Saved file: ");
  Serial.println(WAV_FILE_NAME);

  i2s_driver_uninstall(I2S_PORT);
}

void loop() {
}