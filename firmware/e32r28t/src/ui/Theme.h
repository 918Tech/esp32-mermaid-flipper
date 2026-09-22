#pragma once
#include <Arduino.h>

namespace Theme {
// 918 Technologies // MERMAID
// Deep-ocean base with bioluminescent aqua/cyan, violet highlights,
// and restrained safety colors for physical approval actions.
constexpr uint16_t Abyss      = 0x0026;
constexpr uint16_t DeepSea    = 0x00A9;
constexpr uint16_t Tide       = 0x0190;
constexpr uint16_t Reef       = 0x0333;
constexpr uint16_t Foam       = 0xEFFF;

constexpr uint16_t Bg         = Abyss;
constexpr uint16_t Panel      = DeepSea;
constexpr uint16_t Elevated   = Tide;
constexpr uint16_t Border     = 0x14F5;
constexpr uint16_t Text       = Foam;
constexpr uint16_t Secondary  = 0x8DF7;

constexpr uint16_t Cyan       = 0x07FF;
constexpr uint16_t Aqua       = 0x07F5;
constexpr uint16_t Violet     = 0xA81F;
constexpr uint16_t Pearl      = 0xBFFB;
constexpr uint16_t Green      = 0x07E0;
constexpr uint16_t Amber      = 0xFDC0;
constexpr uint16_t Red        = 0xF800;
}
