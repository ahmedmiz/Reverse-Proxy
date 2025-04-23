# Modern C++ API Gateway

A high-performance API Gateway built in C++ that provides routing, security, and real-time communication capabilities.

## Key Features

- **Dynamic Routing & Load Balancing** - Intelligent request routing with automatic failover
- **Security & Authentication** - JWT validation, IP filtering, SSL termination, and CORS support
- **Performance Optimization** - Response compression and client rate limiting
- **WebSocket Support** - Bidirectional real-time proxy capabilities
- **Monitoring & Observability** - Comprehensive logging and performance metrics

## Tech Stack

- **Core**: C++17, Boost.Asio
- **Security**: OpenSSL, JWT authentication
- **Data Handling**: JsonCpp
- **Performance**: Redis for caching and rate limiting
- **Networking**: WebSocket++, libcurl
- **Build System**: CMake

## Architecture Overview

See the detailed [Architecture Documentation](doc/architecture-documentation.md) for a complete overview of the system design and component interactions.

## Design Decisions

See the detailed [Design Decisisons](doc/design-decisions.md) for a complete
overview of why each Design Decision was taken.

## Code Organization

See the detailed [Code Organization](doc/code-organization.md) for a complete
overview of the structure of the porject.

## Build Instructions

### Prerequisites

- **CMake**: Ensure CMake is installed on your system.
- **Compiler**: A C++ compiler such as `g++` or `clang++`.
- **Dependencies**: Ensure required libraries like `jsoncpp` are installed.

### Steps

1. Clone the repository:

   ```bash
   git clone <repository-url>
   cd RP
   ```

2. Build the project using the build script:

   ```bash
   # Build in Debug mode (default)
   ./build.sh
   
   # Or build in Release mode
   ./build.sh Release
   ```

3. Run the project:

   ```bash
   # Navigate to build directory
   cd build_Debug    # or build_Release if built in Release mode
   
   # Run the reverse proxy
   ./bin/reverse_proxy
   ```

4. Run tests (optional):

   ```bash
   # Either way works:
   ./build.sh test   # Run tests directly
   # OR
   cd build_Debug
   ctest -V         # Run tests from build directory
   ```

## Usage

### Running the Reverse Proxy

After building the project, the `reverse_proxy` binary will be available in the `bin` directory. Run it as follows:

```bash
./bin/reverse_proxy
```

### Running Tests

To execute the unit tests, run the `test_config` binary:

```bash
./bin/test_config
```

## Project Structure

- **src/**: Contains the source code for the reverse proxy and its components.
- **tests/**: Includes unit tests for the project.
- **build_Debug/**: Build directory for the project.
- **CMakeLists.txt**: CMake configuration file.

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository.
2. Create a feature branch.
3. Commit your changes.
4. Submit a pull request.

## Contact

For questions or feedback, please contact:

Name: Ahmed Saad
Email: <mizoahmed017@gmail.com>

GitHub: ahmedmiz
