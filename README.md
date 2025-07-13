# liteMQ

A lightweight, socket-based pub/sub messaging tool written in C.

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