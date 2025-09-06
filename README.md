# BitBots EcoWatt Device - Inverter Communication System

A modular and flexible system for communicating with solar inverters using Modbus protocol over HTTP API. This project provides dynamic polling configuration, comprehensive monitoring capabilities, and a clean, maintainable codebase.

## 🌟 Features

- **Dynamic Polling Configuration**: Configure what parameters to monitor at runtime
- **Modular Architecture**: Clean separation of concerns with reusable components
- **Configuration File System**: External configuration for API keys and endpoints
- **Comprehensive Monitoring**: Support for 10+ inverter parameters
- **Predefined Profiles**: Common monitoring configurations out-of-the-box
- **Thread-Safe Data Handling**: Concurrent polling and data upload
- **Flexible Sample Storage**: Key-value based data structure for extensibility

## 📋 Requirements

### Software Dependencies

- **C++11** or higher compiler (g++)
- **libcurl** development libraries
- **Make** build system

### System Requirements

- macOS, Linux, or Unix-like system
- Network connectivity to inverter API endpoint
- Sufficient permissions to create/read configuration files

### Installation of Dependencies

#### macOS (using Homebrew)

```bash
brew install curl
```

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install libcurl4-openssl-dev build-essential
```

#### CentOS/RHEL

```bash
sudo yum install libcurl-devel gcc-c++ make
```

## 🚀 Quick Start

### 1. Clone and Build

```bash
git clone <repository-url>
cd Milestone_2
make clean && make main
```

### 2. Configure the System

Ensure the `config.ini` file is properly configured with your API credentials:

```ini
[API]
api_key=YOUR_API_KEY_HERE

[ENDPOINTS]
read_url=http://your-api-endpoint/api/inverter/read
write_url=http://your-api-endpoint/api/inverter/write

[DEVICE]
default_slave_address=0x11
```

### 3. Run the Application

```bash
./main
```

## 🏗️ Project Structure

```
Milestone_2/
├── main.cpp                    # Main application entry point
├── config.ini                  # Configuration file
├── Makefile                    # Build configuration
│
├── Core Components/
│   ├── Inverter.{h,cpp}        # High-level inverter interface
│   ├── ModbusHandler.{h,cpp}   # Modbus protocol handling
│   ├── ProtocolAdapter.{h,cpp} # HTTP API communication
│   └── Config.{h,cpp}          # Configuration management
│
├── Polling System/
│   ├── PollingConfig.{h,cpp}   # Dynamic polling configuration
│   └── polling_example.cpp     # Usage examples
│
├── Testing/
│   ├── tests.cpp               # Comprehensive test suite
│   └── test_runner             # Test execution binary
│
└── Documentation/
    ├── CONFIG.md               # Configuration system details
    ├── DYNAMIC_POLLING.md      # Polling system documentation
    └── MODULAR_POLLING.md      # Modular architecture guide
```

## ⚙️ Configuration System

### Configuration File (`config.ini`)

The system uses an INI-format configuration file for external settings:

#### [API] Section

- `api_key`: Authentication key for the inverter API service

#### [ENDPOINTS] Section

- `read_url`: HTTP endpoint for reading inverter data
- `write_url`: HTTP endpoint for writing inverter settings

#### [DEVICE] Section

- `default_slave_address`: Default Modbus slave address (hex format)

### Benefits

- **Security**: API keys stored separately from source code
- **Flexibility**: Easy environment switching without recompilation
- **Modularity**: Configuration changes don't require code modifications

## 📊 Dynamic Polling System

### Available Parameters

The system can monitor these inverter parameters:

| Parameter              | Description             | Unit |
| ---------------------- | ----------------------- | ---- |
| `AC_VOLTAGE`           | AC Phase voltage        | V    |
| `AC_CURRENT`           | AC Phase current        | A    |
| `AC_FREQUENCY`         | AC Phase frequency      | Hz   |
| `PV1_VOLTAGE`          | PV1 input voltage       | V    |
| `PV2_VOLTAGE`          | PV2 input voltage       | V    |
| `PV1_CURRENT`          | PV1 input current       | A    |
| `PV2_CURRENT`          | PV2 input current       | A    |
| `TEMPERATURE`          | Inverter temperature    | °C   |
| `EXPORT_POWER_PERCENT` | Export power percentage | %    |
| `OUTPUT_POWER`         | Inverter output power   | W    |

### Predefined Monitoring Profiles

#### Basic AC Monitoring

```cpp
pollingConfig.setBasicACProfile();
// Monitors: AC voltage, current, frequency
```

#### Comprehensive Monitoring

```cpp
pollingConfig.setComprehensiveProfile();
// Monitors: All AC parameters + thermal + power data
```

#### PV Monitoring

```cpp
pollingConfig.setPVMonitoringProfile();
// Monitors: PV1/PV2 voltage, current + temperature
```

#### Thermal Monitoring

```cpp
pollingConfig.setThermalProfile();
// Monitors: Temperature + output power
```

### Custom Configuration

```cpp
// Custom parameter selection
pollingConfig.setParameters({
    ParameterType::AC_VOLTAGE,
    ParameterType::AC_CURRENT,
    ParameterType::TEMPERATURE
});

// Runtime modifications
pollingConfig.addParameter(ParameterType::OUTPUT_POWER);
pollingConfig.removeParameter(ParameterType::TEMPERATURE);
```

## 🔧 Build System

### Available Make Targets

```bash
make clean          # Clean build artifacts
make main          # Build main application
make tests         # Build test suite
make run           # Build and run main application
make test          # Build and run tests
make all           # Build both main and tests, then run main
```

### Build Process

The build system automatically compiles all required components:

- Core inverter communication modules
- Dynamic polling configuration system
- Configuration management
- External dependencies (libcurl)

## 🧪 Testing

### Running Tests

```bash
make test
```

The test suite includes:

- **Modbus Protocol Tests**: Frame generation and parsing
- **API Communication Tests**: HTTP request/response handling
- **Error Handling Tests**: Invalid frames and error codes
- **Configuration Tests**: Loading and validation

### Test Coverage

- Invalid Modbus frames
- Read-only register write attempts
- Malformed API responses
- Network connectivity issues
- Configuration file validation

## 🎯 Usage Examples

### Basic Application Usage

```cpp
#include "Inverter.h"
#include "PollingConfig.h"

int main() {
    // Create inverter instance (auto-loads configuration)
    Inverter inverter;

    // Configure polling parameters
    PollingConfig config;
    config.setComprehensiveProfile();

    // Start monitoring...
    return 0;
}
```

### Custom Polling Configuration

```cpp
PollingConfig config;

// Start with basic AC monitoring
config.setBasicACProfile();

// Add thermal monitoring
config.addParameter(ParameterType::TEMPERATURE);

// Switch to PV monitoring mode
config.setPVMonitoringProfile();

// View current configuration
config.printEnabledParameters();
```

## 🏃‍♂️ Running the Application

### Standard Execution

```bash
./main
```

### Sample Output

```
=== Inverter Communication Demo ===
Export power set to 20%
Temperature: 61.5 C
Export Power Percent: 20 %
AC Measurements - Voltage: 238.4 V, Current: 14.9 A, Frequency: 50.15 Hz

=== Dynamic Polling Configuration ===
Enabled polling parameters:
  - AC_Voltage (V)
  - AC_Current (A)

=== Starting Dynamic Polling ===
Uploading 5 samples
t=0 ms AC_Voltage=238.4V AC_Current=14.9A
t=6336 ms AC_Voltage=238.4V AC_Current=14.9A
...
```

## 🔬 Architecture Details

### Modular Design

The system follows a layered architecture:

1. **Application Layer** (`main.cpp`): User interface and application logic
2. **Inverter Layer** (`Inverter.cpp`): High-level device abstraction
3. **Protocol Layer** (`ModbusHandler.cpp`): Modbus protocol implementation
4. **Communication Layer** (`ProtocolAdapter.cpp`): HTTP API interface
5. **Configuration Layer** (`Config.cpp`): Settings management

### Data Flow

```
main.cpp → Inverter → ModbusHandler → ProtocolAdapter → HTTP API
                 ↓
         PollingConfig → Sample → DataBuffer → Upload
```

### Thread Safety

- **Polling Thread**: Continuously reads configured parameters
- **Upload Thread**: Periodically uploads buffered data
- **Mutex Protection**: Thread-safe data buffer operations

## 🛠️ Development

### Adding New Parameters

1. **Extend Enum**: Add to `ParameterType` in `PollingConfig.h`
2. **Add Inverter Method**: Implement getter in `Inverter.cpp`
3. **Configure Parameter**: Add to `initializeParameterConfigs()` in `PollingConfig.cpp`

### Creating Custom Profiles

```cpp
void PollingConfig::setMyCustomProfile() {
    setParameters({
        ParameterType::AC_VOLTAGE,
        ParameterType::TEMPERATURE,
        ParameterType::OUTPUT_POWER
    });
}
```

## 🔍 Troubleshooting

### Common Issues

#### Build Errors

- **Missing libcurl**: Install libcurl development packages
- **C++11 not supported**: Update compiler to g++ 4.7+

#### Runtime Errors

- **Config file not found**: Ensure `config.ini` exists in working directory
- **API connection failed**: Check network connectivity and endpoint URLs
- **Authentication failed**: Verify API key in configuration file

#### Performance Issues

- **High CPU usage**: Reduce polling frequency in main.cpp
- **Memory usage**: Adjust buffer capacity in DataBuffer constructor

### Debug Information

Enable verbose output by modifying the polling interval or adding debug prints to track data flow.

## 📄 License

This project is part of the BitBots EcoWatt Device development initiative.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Implement changes with appropriate tests
4. Submit a pull request with detailed description

## 📞 Support

For technical support or questions:

- Review the documentation files in the repository
- Check the test suite for usage examples
- Examine the polling_example.cpp for configuration guidance

---

_This project provides a robust foundation for solar inverter monitoring with extensible architecture and comprehensive functionality._
