#pragma once
#include <Arduino.h>
#include "../ui/UiState.h"
struct Proposal {
  char id[24]{};
  char summary[121]{};
  char target[16]{};
  char command[161]{};
  ProposalRisk risk{ProposalRisk::ReadOnly};
  uint32_t expiresAtMs{0};
  bool valid{false};
  bool handled{false};
};
