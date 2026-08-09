#!/bin/bash
set -e

echo "=========================================="
echo "    Orbit Benchmark Suite (using ab)      "
echo "=========================================="

if ! command -v ab &> /dev/null
then
    echo "Error: 'ab' (apache2-utils) could not be found. Please install it first."
    exit 1
fi

# Ensure server is built in Release mode
make build

# Start the server in the background
echo "Starting basic_server on port 3000..."
./build/basic_server --port 3000 &
SERVER_PID=$!

# Wait for server to start
sleep 2

# Check if server is running
if ! ps -p $SERVER_PID > /dev/null; then
    echo "Server failed to start!"
    exit 1
fi

echo "Server running with PID $SERVER_PID"
echo ""

# Test scenarios
CONCURRENCY=100
REQUESTS=20000

echo "--- Running Plaintext Route Benchmark (/api/v1/users) ---"
ab -c $CONCURRENCY -n $REQUESTS -k http://127.0.0.1:3000/api/v1/users
echo ""

echo "--- Running JSON POST Route Benchmark (/api/v1/users) ---"
# Create a dummy payload file for ab
echo '{"name":"benchmark","age":99}' > /tmp/payload.json
ab -c $CONCURRENCY -n $REQUESTS -k -p /tmp/payload.json -T application/json http://127.0.0.1:3000/api/v1/users
rm /tmp/payload.json
echo ""

echo "Shutting down server..."
kill -SIGINT $SERVER_PID
wait $SERVER_PID || true
echo "Benchmarks completed."
