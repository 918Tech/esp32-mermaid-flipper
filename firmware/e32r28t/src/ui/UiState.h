#pragma once
#include <Arduino.h>
enum class UiPage : uint8_t { Core, Codex, Uart, Scan, Cam };
enum class UiMode : uint8_t { Booting, CalibrationRequired, ConnectingS3, ConnectingFlipper, Ready, Capturing, CodexAnalyzing, ProposalPending, CommandSending, CommandComplete, CommandBlocked, ProposalDenied, Degraded, Error, EmergencyStop };
enum class TouchPhase : uint8_t { Idle, PressCandidate, Pressed, ReleaseCandidate, Released };
enum class LinkState : uint8_t { Ready, Online, SafeMode, Secure, Connecting, Degraded, Offline, Error };
enum class ProposalRisk : uint8_t { ReadOnly, StateChange, RadioTransmit, FirmwareChange };
