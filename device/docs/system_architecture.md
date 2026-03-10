# QuickNote System Architecture
Please note this an AI generated file that is prone to errors/hallucenations. Sections will be rewritten by myself as features are complete/finalised.


## Purpose

QuickNote is a wearable voice note device intended for hands-free note capture.

Primary use case:
- user presses a wrist-mounted button while working
- device records a short voice note
- note is transferred to the user's phone
- phone processes audio into text
- note is displayed and stored in the companion app

---

## High-Level Flow

1. User presses device button
2. Device starts recording audio from onboard microphone
3. Audio is stored locally on device
4. User stops recording
5. Device transfers recorded audio to phone over Bluetooth LE
6. Companion app receives audio file
7. Companion app uploads audio for speech-to-text processing
8. Resulting text note is stored and displayed to user

---

## Current Prototype Scope

Current prototype includes:
- ESP32-S3 development board (strictly as it is what I had on hand, full product will likely contain some sort of Nordic BLE chip)
- INMP441 I2S microphone
- onboard microSD storage
- push-button input

Current implemented / in progress:
- SD card read/write
- microphone sampling
- WAV recording
- button-controlled recording

Not yet implemented:
- BLE file transfer
- phone app
- speech-to-text pipeline
- note persistence / sync

---

## Device Responsibilities

The wearable device is responsible for:
- detecting button input
- recording microphone audio
- saving audio locally
- transferring completed audio files to phone

The device is **not** responsible for:
- speech-to-text processing (might mess around with retraining some super compact model as I will have to have SD for local note storage)
- long-term cloud storage
- note visualisation / editing

These tasks are handled by the phone app / backend.

---

## Companion App Responsibilities

The companion app will be responsible for:
- maintaining Bluetooth connection to the device
- receiving completed audio recordings
- sending audio for transcription
- displaying text notes to the user
- optionally syncing notes to cloud storage

---

## Audio Pipeline

Microphone:
- INMP441
- I2S digital microphone

Current recording format:
- mono
- 16 kHz
- WAV / PCM for prototype simplicity

Likely future direction:
- compressed audio format for faster BLE transfer

---

## Storage Strategy

Current prototype:
- save audio recordings to onboard microSD card

Future behaviour:
- record locally first
- transfer completed recording to phone immediately after recording stops
- optionally delete local file after successful transfer

---

## Bluetooth Strategy

Planned approach:
- Bluetooth Low Energy (BLE)
- device transfers completed recordings to companion app
- no requirement for user to interact with phone during normal use

Important constraint:
- system should work while phone remains in user's pocket
- Android foreground service / persistent notification is acceptable
- iOS behaviour must account for background BLE limitations

---

## Recording Constraints

Target normal recording length:
- up to 60 seconds

Longer notes:
- can be handled as multiple recordings and merged later in software if required

Design goal:
- optimise for fast availability on phone after recording ends

---

## Current Hardware Pin Usage

### INMP441 microphone
- VDD -> 3.3V
- GND -> GND
- SD -> GPIO1
- WS -> GPIO2
- SCK -> GPIO42
- L/R -> GND

### Button
- GPIO14 -> button -> GND
- configured with internal pull-up

### Storage
- onboard microSD slot
- accessed via SD_MMC

---

## Open Questions

- final BLE transfer protocol
- file naming / queueing strategy for multiple pending notes
- deletion policy after successful phone transfer
- battery and charging architecture
- enclosure and final PCB design
- final interaction model:
  - press once to start / press again to stop
  - or hold-to-record

---

## Near-Term Milestones

1. Reliable button-controlled recording
2. Start/stop recording interaction
3. Multiple file handling
4. BLE transfer to phone
5. Basic companion app receiver
6. Speech-to-text integration