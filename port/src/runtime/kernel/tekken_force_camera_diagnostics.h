#pragma once
#include <cstdint>

// Opt-in only. The trace records final native matrices and the actual vertex
// constants consumed by draws; it never changes renderer state.
bool TekkenForceStationaryCamera();
bool TekkenForceLegacyCamera();
bool TekkenForceCameraTraceEnabled();
void TekkenForceTraceCamera(const char* phase, uint8_t* base, uint32_t camera,
    uint32_t player, uint64_t tick, const float* eye, const float* target);
void TekkenForceTraceDraw(const uint32_t* regs, uint32_t shader, uint64_t frame);
