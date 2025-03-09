# High-Performance OrderBook Implementation

A high-performance order book implementation with optimized data structures and multi-connection WebSocket support for cryptocurrency market data processing.

## Key Features

### Features in Detail:
- Real-time order book processing from Gemini exchange
- Full market depth support (100 levels per side)
- Handles trades, cancellations, and new orders
- Automatic price level organization
- Thread-safe message processing
- Memory efficient design for high-frequency updates

### OrderBook Optimizations
- **Bucketed Array Structure**
  - Uses `std::deque<std::list<Order>>` for stable iterators and efficient insertions
  - Price levels grouped into buckets based on price steps (15.00$ increments)
  - O(1) best price lookup
  - O(log k) order placement within bucket, where k is CACHE_DEPTH
  - Memory efficient storage for large number of price levels

- **Price Level Management**
  - Internal uint32_t price representation (2 decimal precision)
  - Efficient bucket shifts for new best prices
  - Automatic bucket cleanup and rebalancing
  - Hash map for O(1) price level lookups

### WebSocket Connection Management
- **Multi-Connection Support**
  - Configurable number of parallel WebSocket connections
  - Load balancing across connections
  - Automatic failover on connection loss
  - Connection-specific sequence tracking

- **Message Deduplication**
  - Ring buffer implementation for memory efficiency
  - O(1) duplicate detection using hash set
  - Periodic memory cleanup with message preservation
  - Connection-aware message sequencing

### Performance Features
- **Lock-Free Message Queue**
  - MPSC (Multi-Producer Single-Consumer) queue
  - Zero-copy message passing
  - Cache-friendly data structures
  - Minimal contention between producers

- **Memory Management**
  - Fixed-size bucket arrays
  - Efficient memory reuse
  - Automatic cleanup of stale data
  - Optimized for high-frequency updates

### Dependencies:
- Boost libraries
- simdjson library

## Build and Installation

### Prerequisites
- C++20 compatible compiler
- Boost libraries
- simdjson library

### Installation
unzip Baltser_Anton_Gemini_OrderBook.zip
cd Baltser_Anton_Gemini_OrderBook
make directories
make -j 8 libraries
make -j 8

# Recommended: 2-4 connections for optimal performance

### Run:

./build/main
type number of connections for WS



