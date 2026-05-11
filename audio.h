// audio.h
// Audio subsystem for the ESP32-S3-Box-3: ES8311 codec init + I²S DMA +
// SN76489-faithful tone/noise generator. CALL SOUND from BASIC routes
// here via the strong tiSoundPlay / tiSoundStop overrides.
#pragma once

// Bring up the I²S bus, configure the ES8311 codec, enable the power
// amp on GPIO46, and start the audio mixing task. Idempotent.
void initAudio();
