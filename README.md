# Networking App

Networking App is a small C++ learning project that demonstrates client-server communication through [GameNetworkingSockets](https://github.com/valvesoftware/gamenetworkingsockets). It contains a reusable client library, a reusable server library, and a console application that can be launched either as a server or as a client from command-line arguments.

## Demo

![Networking App demo](assets/networking-app.gif)

## Features

- Client-server networking through [GameNetworkingSockets](https://github.com/valvesoftware/gamenetworkingsockets).
- One executable with command-line mode selection.
- Server mode with configurable listen port.
- Client mode with configurable server address.
- Callback-based connection, disconnection, and packet receive handling.
- Periodic packet sending every 3 seconds from client to server.
- Periodic packet broadcasting every 3 seconds from server to all connected clients.
- Simple `Buffer` abstraction for sending raw packet data.
- `fmt` is used for formatted runtime error messages.
- Required runtime DLL files are copied next to the executable during the CMake build.

## Requirements

This project is currently set up for Windows.

Install the following tools before building:

1. **Visual Studio 2022** or **Visual Studio Build Tools 2022**
   - Install the `Desktop development with C++` workload.
   - Make sure the MSVC compiler and Windows SDK components are selected.
2. **CMake 3.20 or newer**
   - Download it from <https://cmake.org/download/> or install it with WinGet:

   ```powershell
   winget install Kitware.CMake
   ```

3. **Git**
   - Required if you want to clone the repository from the command line.

   ```powershell
   winget install Git.Git
   ```

## Source Layout

```text
assets/                    Demo media used by this README
project/
  CMakeLists.txt           Main CMake project
  app/                     Console application entry point and runtime DLL resources
  client/                  Client library source and public headers
  server/                  Server library source and public headers
  third_party/             Vendored fmt, GameNetworkingSockets headers/lib, and buffer helper
```

## Build Tutorial

Run the following commands from the repository root.

Create the build directory and generate the build system. This step only needs to be done once:

```powershell
cmake -S project -B build
```

Build the Release configuration:

```powershell
cmake --build build --config Release
```

After a successful Visual Studio generator build, the executable is typically located at:

```text
build/app/Release/networking-app.exe
```

The build also copies the required runtime DLL files next to the executable.

## Running

Open two terminals from the repository root.

Start the server in the first terminal:

```powershell
build\app\Release\networking-app.exe server
```

You can also provide a custom port:

```powershell
build\app\Release\networking-app.exe server 27020
```

Start the client in the second terminal and pass the server address:

```powershell
build\app\Release\networking-app.exe client 127.0.0.1:27020
```

While both processes are running:

- the client sends a packet to the server every 3 seconds;
- the server sends a packet to all connected clients every 3 seconds;
- received packets and connection events are printed to the console.

Press `Enter` in either terminal to stop that process.

## Development Notes

- The application target is defined in `project/app/CMakeLists.txt`.
- The application entry point is implemented in `project/app/src/main.cpp`.
- The client API is declared in `project/client/include/client.h` and implemented in `project/client/src/client.cpp`.
- The server API is declared in `project/server/include/server.h` and implemented in `project/server/src/server.cpp`.
- Vendored dependencies are included through `project/third_party`, including [GameNetworkingSockets](https://github.com/valvesoftware/gamenetworkingsockets).
- Runtime DLL resources are stored in `project/app/resources` and copied next to `networking-app.exe` during the CMake build.
