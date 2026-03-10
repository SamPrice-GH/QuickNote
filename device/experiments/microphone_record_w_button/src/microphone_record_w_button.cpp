#include <Arduino.h>
#include <driver/i2s.h>
#include "FS.h"
#include "SD_MMC.h"

#define I2S_PORT I2S_NUM_0

// Microphone pins
#define I2S_SD   1
#define I2S_WS   2
#define I2S_SCK  42

// PRESET SD MMC PINS - DO NOT CHANGE
#define SD_MMC_CMD 38 
#define SD_MMC_CLK 39  
#define SD_MMC_D0  40 

// Button pin
#define BUTTON_PIN 14

#define SAMPLE_RATE     16000
#define BITS_PER_SAMPLE 16
#define RECORD_SECONDS  5

bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void writeWavHeader(File &file, uint32_t dataSize) {
  uint32_t fileSize = dataSize + 36;
  uint16_t audioFormat = 1;
  uint16_t numChannels = 1;
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

String getNextFilename() {
  for (int i = 1; i < 1000; i++) {
    char filename[20];
    snprintf(filename, sizeof(filename), "/note%03d.wav", i);
    if (!SD_MMC.exists(filename)) {
      return String(filename);
    }
  }
  return "/overflow.wav";
}

void recordWav(const char *filename) {
  File file = SD_MMC.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open WAV file for writing");
    return;
  }

  uint8_t emptyHeader[44] = {0};
  file.write(emptyHeader, 44);

  const size_t samplesPerRead = 256;
  int32_t i2sBuffer[samplesPerRead];
  int16_t pcmBuffer[samplesPerRead];

  uint32_t totalBytesWritten = 0;
  uint32_t totalReads = (SAMPLE_RATE * RECORD_SECONDS) / samplesPerRead;

  Serial.print("Recording to ");
  Serial.println(filename);

  for (uint32_t r = 0; r < totalReads; r++) {
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, i2sBuffer, sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);

    int count = bytesRead / sizeof(int32_t);

    for (int i = 0; i < count; i++) {
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
}

bool buttonPressedEvent() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
    lastButtonState = reading;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    static bool buttonHandled = false;

    if (reading == LOW && !buttonHandled) {
      buttonHandled = true;
      return true;
    }

    if (reading == HIGH) {
      buttonHandled = false;
    }
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Mounting SD card...");
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    Serial.println("Card Mount Failed");
    return;
  }

  Serial.println("Initialising I2S...");
  setupI2S();

  Serial.println("Ready. Press button to record 5 seconds.");
}

void loop() {
  if (buttonPressedEvent()) {
    String filename = getNextFilename();
    recordWav(filename.c_str());
    Serial.println("Ready for next recording.");
  }
}