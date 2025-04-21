# Reverse Proxy Project

## Overview

This project implements a reverse proxy server designed to handle HTTP requests, load balancing, and security features such as authentication and SSL. It also includes a configuration module and unit tests to ensure reliability.

## Features

- **HTTP Server**: Handles incoming HTTP requests and responses.
- **Load Balancer**: Distributes traffic across multiple backend servers.
- **Security**: Includes JWT-based authentication and SSL support.
- **Logging and Error Handling**: Provides robust logging and error management.
- **Configuration**: Modular configuration system for easy customization.
- **Unit Tests**: Comprehensive tests for critical components.

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

2. Create a build directory:

   ```bash
   mkdir build_Debug
   cd build_Debug
   ```

3. Configure the project:

   ```bash
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   ```

4. Build the project:

   ```bash
   make
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

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Contact

For questions or feedback, please contact the project maintainer.
