#!/bin/bash
set -e

echo "Building project..."
cmake --build build

echo ""
echo "=========================================="
echo "    BENCHMARKING EPOLL (Reactor)          "
echo "=========================================="
./build/basic_server --engine epoll > /dev/null 2>&1 &
SERVER_PID=$!
sleep 1 # Wait for server to start

echo "Running warmup..."
npx -y autocannon -c 10 -d 2 http://localhost:8080/ > /dev/null

echo "Running Epoll benchmark..."
npx -y autocannon -c 100 -p 10 -d 10 http://localhost:8080/

kill $SERVER_PID
sleep 1

echo ""
echo "=========================================="
echo "    BENCHMARKING IO_URING (Poll Mode)     "
echo "=========================================="
./build/basic_server --engine iouring > /dev/null 2>&1 &
SERVER_PID=$!
sleep 1 # Wait for server to start

echo "Running warmup..."
npx -y autocannon -c 10 -d 2 http://localhost:8080/ > /dev/null

echo "Running io_uring benchmark..."
npx -y autocannon -c 100 -p 10 -d 10 http://localhost:8080/

kill $SERVER_PID
echo "Done!"
