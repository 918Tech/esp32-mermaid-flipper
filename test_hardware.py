"""Hardware testing utilities for ESP32-Mermaid-Flipper system."""

import time
import logging
from typing import Dict, List, Optional, Tuple
from enum import Enum
from dataclasses import dataclass, field


class DeviceStatus(Enum):
    """Status of connected devices."""
    DISCONNECTED = "disconnected"
    CONNECTING = "connecting"
    CONNECTED = "connected"
    ERROR = "error"
    TESTING = "testing"
    READY = "ready"


class HardwareTestError(Exception):
    """Base exception for hardware testing errors."""
    pass


@dataclass
class DeviceTestResult:
    """Result of a device test."""
    device_name: str
    device_type: str  # 's3cam', 'e32r28t', 'flipper'
    status: DeviceStatus
    test_timestamp: float = field(default_factory=time.time)
    tests_passed: int = 0
    tests_failed: int = 0
    errors: List[str] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)
    diagnostic_info: Dict = field(default_factory=dict)
    
    def success(self) -> bool:
        """Check if all tests passed."""
        return self.tests_failed == 0 and self.status == DeviceStatus.READY


class HardwareTester:
    """Base class for hardware testing."""
    
    def __init__(self, device_name: str, device_type: str, timeout: float = 10.0):
        self.device_name = device_name
        self.device_type = device_type
        self.timeout = timeout
        self.logger = self._setup_logger()
        self.status = DeviceStatus.DISCONNECTED
        self.last_heartbeat = None
    
    def _setup_logger(self) -> logging.Logger:
        """Setup logging for this device."""
        logger = logging.getLogger(f'HardwareTest-{self.device_name}')
        handler = logging.StreamHandler()
        formatter = logging.Formatter(
            f'[{self.device_name}] %(levelname)s: %(message)s'
        )
        handler.setFormatter(formatter)
        logger.addHandler(handler)
        logger.setLevel(logging.DEBUG)
        return logger
    
    def connect(self) -> bool:
        """Attempt to connect to device."""
        self.logger.info(f"Connecting to {self.device_name}...")
        self.status = DeviceStatus.CONNECTING
        
        try:
            # Override in subclasses
            self._connect_impl()
            self.status = DeviceStatus.CONNECTED
            self.logger.info(f"Connected successfully")
            return True
        except Exception as e:
            self.status = DeviceStatus.ERROR
            self.logger.error(f"Connection failed: {e}")
            return False
    
    def _connect_impl(self):
        """Implement connection logic in subclasses."""
        raise NotImplementedError("Subclasses must implement _connect_impl")
    
    def disconnect(self) -> bool:
        """Disconnect from device."""
        self.logger.info(f"Disconnecting from {self.device_name}...")
        try:
            self._disconnect_impl()
            self.status = DeviceStatus.DISCONNECTED
            self.logger.info("Disconnected")
            return True
        except Exception as e:
            self.logger.error(f"Disconnect failed: {e}")
            return False
    
    def _disconnect_impl(self):
        """Implement disconnection logic in subclasses."""
        pass
    
    def heartbeat(self) -> bool:
        """Send heartbeat to device."""
        if self.status != DeviceStatus.CONNECTED:
            self.logger.warning(f"Cannot heartbeat: status is {self.status}")
            return False
        
        try:
            self._heartbeat_impl()
            self.last_heartbeat = time.time()
            return True
        except Exception as e:
            self.logger.error(f"Heartbeat failed: {e}")
            self.status = DeviceStatus.ERROR
            return False
    
    def _heartbeat_impl(self):
        """Implement heartbeat logic in subclasses."""
        pass
    
    def run_tests(self) -> DeviceTestResult:
        """Run all tests for this device."""
        self.status = DeviceStatus.TESTING
        result = DeviceTestResult(
            device_name=self.device_name,
            device_type=self.device_type,
            status=self.status
        )
        
        self.logger.info(f"Starting test suite for {self.device_name}")
        
        # Connection test
        if not self.connect():
            result.tests_failed += 1
            result.errors.append("Failed to connect to device")
            result.status = DeviceStatus.ERROR
            return result
        
        result.tests_passed += 1
        
        try:
            # Heartbeat test
            if self.heartbeat():
                result.tests_passed += 1
            else:
                result.tests_failed += 1
                result.errors.append("Heartbeat failed")
            
            # Device-specific tests
            device_results = self.run_device_tests()
            result.tests_passed += device_results.get('passed', 0)
            result.tests_failed += device_results.get('failed', 0)
            result.errors.extend(device_results.get('errors', []))
            result.warnings.extend(device_results.get('warnings', []))
            result.diagnostic_info.update(device_results.get('diagnostics', {}))
        
        finally:
            self.disconnect()
        
        result.status = DeviceStatus.READY if result.success() else DeviceStatus.ERROR
        self.status = result.status
        
        self.logger.info(
            f"Test complete: {result.tests_passed} passed, "
            f"{result.tests_failed} failed"
        )
        
        return result
    
    def run_device_tests(self) -> Dict:
        """Run device-specific tests. Override in subclasses."""
        return {'passed': 0, 'failed': 0, 'errors': [], 'warnings': [], 'diagnostics': {}}


class ESP32S3CameraTester(HardwareTester):
    """Tester for ESP32-S3 camera board."""
    
    def __init__(self, port: str = "/dev/ttyUSB0", baud: int = 115200):
        super().__init__("ESP32-S3-Camera", "s3cam")
        self.port = port
        self.baud = baud
        self.serial_conn = None
    
    def _connect_impl(self):
        """Connect to ESP32-S3 via serial."""
        try:
            import serial
            self.serial_conn = serial.Serial(self.port, self.baud, timeout=5)
            time.sleep(2)  # Wait for device to initialize
        except ImportError:
            raise HardwareTestError("pyserial not installed")
        except Exception as e:
            raise HardwareTestError(f"Serial connection failed: {e}")
    
    def _disconnect_impl(self):
        """Disconnect from ESP32-S3."""
        if self.serial_conn:
            self.serial_conn.close()
    
    def _heartbeat_impl(self):
        """Send heartbeat command to ESP32-S3."""
        if not self.serial_conn:
            raise HardwareTestError("Not connected")
        self.serial_conn.write(b'PING\n')
        response = self.serial_conn.readline()
        if b'PONG' not in response:
            raise HardwareTestError("Invalid heartbeat response")
    
    def run_device_tests(self) -> Dict:
        """Run ESP32-S3 specific tests."""
        results = {'passed': 0, 'failed': 0, 'errors': [], 'warnings': [], 'diagnostics': {}}
        
        try:
            # Test camera initialization
            self.serial_conn.write(b'CAM_INIT\n')
            response = self.serial_conn.readline()
            if b'OK' in response:
                results['passed'] += 1
            else:
                results['failed'] += 1
                results['errors'].append("Camera initialization failed")
            
            # Test UART communication
            self.serial_conn.write(b'UART_TEST\n')
            response = self.serial_conn.readline()
            if b'OK' in response:
                results['passed'] += 1
            else:
                results['failed'] += 1
                results['errors'].append("UART test failed")
            
            # Get device info
            self.serial_conn.write(b'INFO\n')
            info_data = self.serial_conn.readline().decode('utf-8', errors='ignore')
            results['diagnostics']['device_info'] = info_data
            results['passed'] += 1
        
        except Exception as e:
            results['failed'] += 1
            results['errors'].append(f"Device test error: {e}")
        
        return results


class ESP32WROOM32ETester(HardwareTester):
    """Tester for ESP32-WROOM-32E touchscreen approval console."""
    
    def __init__(self, port: str = "/dev/ttyUSB1", baud: int = 115200):
        super().__init__("ESP32-WROOM-32E", "e32r28t")
        self.port = port
        self.baud = baud
        self.serial_conn = None
    
    def _connect_impl(self):
        """Connect to ESP32-WROOM-32E via serial."""
        try:
            import serial
            self.serial_conn = serial.Serial(self.port, self.baud, timeout=5)
            time.sleep(2)
        except ImportError:
            raise HardwareTestError("pyserial not installed")
        except Exception as e:
            raise HardwareTestError(f"Serial connection failed: {e}")
    
    def _disconnect_impl(self):
        """Disconnect from ESP32-WROOM-32E."""
        if self.serial_conn:
            self.serial_conn.close()
    
    def _heartbeat_impl(self):
        """Send heartbeat to approval console."""
        if not self.serial_conn:
            raise HardwareTestError("Not connected")
        self.serial_conn.write(b'PING\n')
        response = self.serial_conn.readline()
        if b'PONG' not in response:
            raise HardwareTestError("Invalid heartbeat response")
    
    def run_device_tests(self) -> Dict:
        """Run approval console specific tests."""
        results = {'passed': 0, 'failed': 0, 'errors': [], 'warnings': [], 'diagnostics': {}}
        
        try:
            # Test touchscreen
            self.serial_conn.write(b'TOUCH_TEST\n')
            response = self.serial_conn.readline()
            if b'OK' in response:
                results['passed'] += 1
            else:
                results['failed'] += 1
                results['errors'].append("Touchscreen test failed")
            
            # Test approval workflow
            self.serial_conn.write(b'APPROVAL_TEST\n')
            response = self.serial_conn.readline()
            if b'OK' in response:
                results['passed'] += 1
            else:
                results['failed'] += 1
                results['errors'].append("Approval workflow test failed")
            
            # Test display
            self.serial_conn.write(b'DISPLAY_TEST\n')
            response = self.serial_conn.readline()
            if b'OK' in response:
                results['passed'] += 1
            else:
                results['failed'] += 1
                results['errors'].append("Display test failed")
        
        except Exception as e:
            results['failed'] += 1
            results['errors'].append(f"Device test error: {e}")
        
        return results


class FlipperZeroTester(HardwareTester):
    """Tester for Flipper Zero device."""
    
    def __init__(self, port: str = "/dev/ttyUSB2", baud: int = 230400):
        super().__init__("Flipper-Zero", "flipper")
        self.port = port
        self.baud = baud
        self.serial_conn = None
    
    def _connect_impl(self):
        """Connect to Flipper Zero via serial."""
        try:
            import serial
            self.serial_conn = serial.Serial(self.port, self.baud, timeout=5)
            time.sleep(2)
        except ImportError:
            raise HardwareTestError("pyserial not installed")
        except Exception as e:
            raise HardwareTestError(f"Serial connection failed: {e}")
    
    def _disconnect_impl(self):
        """Disconnect from Flipper Zero."""
        if self.serial_conn:
            self.serial_conn.close()
    
    def _heartbeat_impl(self):
        """Send heartbeat to Flipper Zero."""
        if not self.serial_conn:
            raise HardwareTestError("Not connected")
        self.serial_conn.write(b'PING\n')
        response = self.serial_conn.readline()
        if b'PONG' not in response:
            raise HardwareTestError("Invalid heartbeat response")
    
    def run_device_tests(self) -> Dict:
        """Run Flipper Zero specific tests."""
        results = {'passed': 0, 'failed': 0, 'errors': [], 'warnings': [], 'diagnostics': {}}
        
        try:
            # Test UART connection
            self.serial_conn.write(b'UART_TEST\n')
            response = self.serial_conn.readline()
            if b'OK' in response:
                results['passed'] += 1
            else:
                results['failed'] += 1
                results['errors'].append("UART test failed")
            
            # Verify allowlist
            self.serial_conn.write(b'LIST_ALLOWLIST\n')
            allowlist = self.serial_conn.readline().decode('utf-8', errors='ignore')
            if 'loader list' in allowlist.lower():
                results['passed'] += 1
                results['diagnostics']['allowlist'] = allowlist
            else:
                results['warnings'].append("Allowlist may not be properly configured")
            
            # Test protocol version
            self.serial_conn.write(b'PROTOCOL_VERSION\n')
            version = self.serial_conn.readline().decode('utf-8', errors='ignore')
            results['passed'] += 1
            results['diagnostics']['protocol_version'] = version
        
        except Exception as e:
            results['failed'] += 1
            results['errors'].append(f"Device test error: {e}")
        
        return results


class HardwareTestSuite:
    """Orchestrates testing of all hardware components."""
    
    def __init__(self):
        self.logger = self._setup_logger()
        self.testers: List[HardwareTester] = []
        self.results: List[DeviceTestResult] = []
    
    def _setup_logger(self) -> logging.Logger:
        logger = logging.getLogger('HardwareTestSuite')
        handler = logging.StreamHandler()
        formatter = logging.Formatter('[SUITE] %(levelname)s: %(message)s')
        handler.setFormatter(formatter)
        logger.addHandler(handler)
        logger.setLevel(logging.INFO)
        return logger
    
    def add_device(self, tester: HardwareTester) -> None:
        """Add device tester to suite."""
        self.testers.append(tester)
        self.logger.info(f"Added {tester.device_name} to test suite")
    
    def run_all_tests(self) -> Dict:
        """Run tests for all devices."""
        self.logger.info(f"Starting test suite for {len(self.testers)} device(s)")
        self.results = []
        
        for tester in self.testers:
            result = tester.run_tests()
            self.results.append(result)
        
        summary = self._generate_summary()
        self.logger.info(f"Test suite complete: {summary}")
        return summary
    
    def _generate_summary(self) -> Dict:
        """Generate test summary."""
        total_passed = sum(r.tests_passed for r in self.results)
        total_failed = sum(r.tests_failed for r in self.results)
        all_success = all(r.success() for r in self.results)
        
        return {
            'total_passed': total_passed,
            'total_failed': total_failed,
            'devices_tested': len(self.results),
            'all_success': all_success,
            'devices': [
                {
                    'name': r.device_name,
                    'type': r.device_type,
                    'success': r.success(),
                    'passed': r.tests_passed,
                    'failed': r.tests_failed,
                    'errors': r.errors
                }
                for r in self.results
            ]
        }
    
    def get_report(self) -> str:
        """Generate human-readable test report."""
        report = "\n" + "="*60 + "\n"
        report += "HARDWARE TEST REPORT\n"
        report += "="*60 + "\n\n"
        
        for result in self.results:
            status_str = "✓ PASS" if result.success() else "✗ FAIL"
            report += f"{result.device_name:30} [{status_str}]\n"
            report += f"  Tests Passed: {result.tests_passed}\n"
            report += f"  Tests Failed: {result.tests_failed}\n"
            
            if result.errors:
                report += "  Errors:\n"
                for error in result.errors:
                    report += f"    - {error}\n"
            
            if result.warnings:
                report += "  Warnings:\n"
                for warning in result.warnings:
                    report += f"    - {warning}\n"
            
            report += "\n"
        
        report += "="*60 + "\n"
        return report
