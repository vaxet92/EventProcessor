# Event Processor

A high-performance, lock-free event processing system implemented in C++20. The system provides efficient event handling through a lock-free ring buffer implementation, making it suitable for concurrent producer-consumer scenarios.

## Features

- Lock-free ring buffer implementation
- Zero-copy event processing
- Thread-safe event handling
- Configurable buffer size (default: 4096 events)
- Memory-efficient event allocation
- Support for multiple producers and single consumer
- RAII-based resource management

## Components

### Ring Buffer (`src/ring_buffer.h`)

The core component implementing a lock-free circular buffer with the following features:
- Thread-safe operations using atomic operations
- Contiguous space reservation for batch operations
- Efficient memory reuse
- Sequence locking for concurrent access
- Memory ordering guarantees

### Event Processor (`src/event_proc.h`)

Manages event lifecycle and processing:
- Event reservation and commitment
- Batch event processing
- Automatic resource cleanup
- Event type safety
- Error handling and reporting

## Building

### Prerequisites

- CMake 3.10 or higher
- C++20 compatible compiler
- pthread library

### Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure CMake
cmake ..

# Build
cmake --build .
```

### Build Types

- Debug (default): Includes debug symbols, sanitizers, and additional checks
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

- Release: Optimized build for production use
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```


## Performance Considerations

- The ring buffer is optimized for high-throughput scenarios
- Uses atomic operations to avoid locks
- Minimizes false sharing through proper memory alignment
- Provides batch operations for improved efficiency
- Supports zero-copy event processing

## Memory Management

The system uses a combination of strategies for efficient memory management:
- RAII for automatic resource cleanup
- Smart pointers for ownership management
- Explicit memory management for performance-critical paths
- Automatic cleanup of uncommitted events

## Thread Safety

The implementation ensures thread safety through:
- Atomic operations
- Memory barriers
- Sequence locking for complex operations
- Lock-free algorithms

## Limitations

- Single consumer design
- Fixed buffer size after initialization
- Events must be pointer-sized
- No support for priority queues
