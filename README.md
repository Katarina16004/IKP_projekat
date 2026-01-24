# Tic Tac Toe Network Game

## Overview

This project is a networked Tic Tac Toe game implemented in C++ using WinSock2 for TCP communication. 
It features a custom binary protocol with checksum validation and supports multiple concurrent users playing over a local network (LAN).

The architecture uses a multi-threaded server that handles connection requests, matchmaking, and game logic for multiple clients in parallel. 
The client application provides a console interface for players to register, see the board, play moves, and receive results. 
Unit tests and a stress-testing tool are included to ensure correctness and stability under high load.

## Features

- Multi-user client-server architecture
- Custom binary protocol (with checksum for error detection)
- Server-side matchmaking and game logic
- Robust handling of connection errors, timeouts, and game interruption
- Responsive client thanks to background listener thread
- Console UI for username entry, board display, and input validation
- Unit tests for core data structures and message handling
- Automated stress tests for scalability and robustness

## Technologies Used

- **C++**
- **WinSock2 library**
- **Windows OS**
- **Google Test**
- **Microsoft Visual Studio**

## Building

1. **Environment:** Windows (Visual Studio 2019+/MSVC), WinSock2 required.
2. **Build all projects/solutions** using Visual Studio
   
## Usage

1. Start the server
2. Run one or more clients
3. Follow on-screen instructions to enter a username and play.

## Unit Testing

Unit tests are provided (using Google Test) for:

- Packet serialization/deserialization and checksum verification
- List data structure correctness
- Protocol error handling

## Stress Testing

A stress testing utility automatically spawns multiple simulated clients to test server stability and matchmaking logic under high load.

## Documentation

Full Serbian project documentation is available.
It contains the following chapters:

- **Introduction:** Problem statement and project objectives.
- **Design:** Description of the implemented design, component diagram, and justification for design choices.
- **Data Structures:** Explanation of why specific data structures were used, with descriptions and semantics of their contents.
- **Testing Results:** Description and rationale of the implemented tests, along with test results (tables, diagrams).
- **Conclusion:** Main conclusions based on test results and discussion of why outcomes are better or worse than expected.
- **Potential Improvements:** Proposed enhancements based on observed limitations, with explanations.
