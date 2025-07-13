# liteMQ

**liteMQ: A Lightweight C-based Pub/Sub Messaging Tool**

  liteMQ is a highly efficient, socket-based publish/subscribe messaging system implemented entirely in C.

  Designed for minimal footprint and high performance, it offers a simple API for message exchange between

  publishers and subscribers.

 Key Features:
 
   * Ultra-Lightweight: Built with standard C libraries, ensuring a small memory footprint and fast execution.

   * Socket-Based Pub/Sub: Enables real-time message broadcasting to multiple subscribers over TCP/IP.

   * Configurable Persistence:

       * No Persistence: Messages are delivered only to active subscribers.

       * Persist All: All messages are saved and delivered to new subscribers upon connection.

       * Timed Persistence: Messages are saved for a configurable duration, with automatic cleanup of expired

         messages.

   * Simple API: Straightforward command-line interfaces for both publishing and subscribing.

   * Robust & Tested: Includes improved error handling and comprehensive unit test coverage for reliability.

  liteMQ is ideal for embedded systems, IoT devices, or any application where a full-fledged message broker is

  overkill, and a fast, simple, and customizable messaging solution is required.


## Features

*   **Lightweight and Fast:** Built with standard C libraries for minimal footprint and maximum performance.
*   **Simple API:** Easy-to-use command-line interface for publishing and subscribing.
*   **Configurable Persistence:** Choose between no persistence, persisting all messages, or persisting messages for a specific duration.
*   **Robust:** Improved error handling and comprehensive unit test coverage.

## Building

To build the server and clients, run:

```bash
make
```

## Usage

### Server

To run the server with no persistence:

```bash
./server
```

To run the server with all messages persisted:

```bash
./server --persist-all
```

To run the server with messages persisted for 60 seconds:

```bash
./server --persist-timed 60
```

### Publisher

To publish a message to a topic:

```bash
./publisher <topic> "Your message here"
```

### Subscriber

To subscribe to a topic:

```bash
./subscriber <topic>
```

## Testing

To run all unit tests:

```bash
make test
```

## Linting and Static Analysis

To run `clang-tidy` for linting and static analysis (requires `bear` and `clang-tidy`):

```bash
make lint
```

## Code Coverage

To generate a code coverage report (requires `gcov`):

```bash
make coverage
```

## Documentation (Doxygen)

To generate HTML documentation using Doxygen:

```bash
make docs
```

This will create an `html` directory containing the generated documentation.

## Author

Mohammed Uddin
