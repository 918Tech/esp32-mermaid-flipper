#!/usr/bin/env python3
"""Example hardware testing script for ESP32-Mermaid-Flipper system."""

import sys
from test_hardware import (
    ESP32S3CameraTester,
    ESP32WROOM32ETester,
    FlipperZeroTester,
    HardwareTestSuite,
)


def main():
    """Run hardware tests."""
    print("\n" + "="*60)
    print("ESP32-Mermaid-Flipper Hardware Test Suite")
    print("="*60 + "\n")
    
    # Create test suite
    suite = HardwareTestSuite()
    
    # Add devices to test
    # Note: Adjust port numbers based on your system configuration
    try:
        # ESP32-S3 Camera
        print("[*] Adding ESP32-S3 Camera tester...")
        s3_tester = ESP32S3CameraTester(port="/dev/ttyUSB0", baud=115200)
        suite.add_device(s3_tester)
    except Exception as e:
        print(f"[!] Warning: Could not add S3 tester: {e}")
    
    try:
        # ESP32-WROOM-32E Approval Console
        print("[*] Adding ESP32-WROOM-32E tester...")
        e32_tester = ESP32WROOM32ETester(port="/dev/ttyUSB1", baud=115200)
        suite.add_device(e32_tester)
    except Exception as e:
        print(f"[!] Warning: Could not add E32 tester: {e}")
    
    try:
        # Flipper Zero
        print("[*] Adding Flipper Zero tester...")
        flipper_tester = FlipperZeroTester(port="/dev/ttyUSB2", baud=230400)
        suite.add_device(flipper_tester)
    except Exception as e:
        print(f"[!] Warning: Could not add Flipper tester: {e}")
    
    if len(suite.testers) == 0:
        print("[!] Error: No devices configured for testing")
        return 1
    
    # Run all tests
    print(f"\n[*] Running tests for {len(suite.testers)} device(s)...\n")
    summary = suite.run_all_tests()
    
    # Print detailed report
    print(suite.get_report())
    
    # Print JSON summary
    import json
    print("\nJSON Summary:")
    print(json.dumps(summary, indent=2))
    
    # Return exit code based on test results
    return 0 if summary['all_success'] else 1


if __name__ == "__main__":
    sys.exit(main())
