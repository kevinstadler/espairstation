#include "colors.h"

// threshold arrays hold *inclusive* **upper** bounds (and their colours)

// temperature-specific
#define VERYCOLD ADAColor(0x0000AA)
#define COLD ADAColor(0x4444CC)
#define SLIGHTLYCOLD ADAColor(0x7777FF)
const float TEMPERATURE[] = { 4, 10, 18, 20, 25, 28, 31, 34, 37, 100 };
const uint16_t TEMPERATURE_COLORS[] = { WHITE, VERYCOLD, COLD, SLIGHTLYCOLD, VERYCOMFY, COMFY, OK, YELLOWISH, ORANGE, DARKORANGE };

// very dry, dry, comfy, ok, uncomfy, horrible
const float DEWPOINT[] = { 5, 10, 15, 20, 100 };
const uint16_t DEWPOINT_COLORS[] = { VERYDRY, DRY, VERYCOMFY, COMFY, YELLOWISH, ORANGE, DARKORANGE };

const float CAQI[] = { 15, 30, 55, 110, 500 };
const uint16_t CAQI_COLORS[] = { ADAColor(0x79BC6A), ADAColor(0xBBCF4C), ADAColor(0xEEC20B), ADAColor(0xF29305), ADAColor(0xE8416F) };
