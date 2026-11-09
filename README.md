# Real-Time Market Data Monitoring System

A C++-based market monitoring application integrating **Apache Kafka, Redis, and Qt 6** to simulate, process, store, and display real-time stock market data.

## Architecture

```text
C++ Kafka Producer
        |
        v
Apache Kafka (market-data)
        |
        v
C++ Kafka Consumer
        |
        v
Redis (Hiredis)
        |
        v
Qt 6 Dashboard
```

## Technologies Used

- **Language:** C++17
- **Messaging:** Apache Kafka, librdkafka
- **Database / Cache:** Redis, Hiredis
- **GUI:** Qt 6 Widgets
- **Build System:** CMake
- **Environment:** macOS, Docker

## Project Structure

- `main.cpp`, `mainwindow.cpp`, `mainwindow.h`, `mainwindow.ui` – Qt dashboard.
- `CMakeLists.txt` – Qt application build configuration.
- `Redis_Server_connection/market_producer.cpp` – Generates simulated stock prices and volumes and publishes messages to Kafka.
- `Redis_Server_connection/market_consumer.cpp` – Consumes Kafka messages and updates stock data in Redis.

## Features

- Generates simulated stock market data for INFOSYS and TCS.
- Publishes market updates to an Apache Kafka topic.
- Consumes Kafka messages using a C++ consumer.
- Stores the latest stock prices and volumes in Redis.
- Displays stock information through a Qt Widgets dashboard.
- Automatically refreshes dashboard data every second.

## Data Flow

The producer generates stock data in the format:

`SYMBOL,PRICE,VOLUME`

The Kafka consumer processes each message and stores the corresponding stock information in Redis hashes, using keys such as `stock:INFOSYS` and `stock:TCS`.

The Qt dashboard periodically retrieves the latest values from Redis and updates its stock table.

## Prerequisites

- C++17-compatible compiler
- CMake
- Qt 6 Widgets
- Apache Kafka
- Redis
- librdkafka
- Hiredis

## Running the Project

1. Start the Redis server.
2. Start the Kafka broker and ensure the `market-data` topic is available.
3. Build and run the Kafka consumer.
4. Run the Kafka producer to publish simulated stock updates.
5. Build and launch the Qt dashboard using Qt Creator or CMake.

## Purpose

This project demonstrates practical integration of C++, message streaming, Redis-based data storage, and GUI-based market monitoring.

## Dashboard Preview

![Market Monitor Dashboard](screenshots/market-monitor-dashboard.png)
