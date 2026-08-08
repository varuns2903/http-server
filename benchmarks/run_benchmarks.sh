#!/bin/bash

# Simple script to benchmark the Orbit HTTP server using `wrk`
# Usage: ./run_benchmarks.sh

set -e

if ! command -v wrk &> /dev/null
then
    echo "Error: 'wrk' could not be found. Please install it first."
    echo "Ubuntu/Debian: sudo apt-get install wrk"
    echo "macOS: brew install wrk"
    exit 1
fi

echo "======================================"
echo " Starting Orbit Benchmark Suite..."
echo "======================================"

echo "Testing static file performance (assuming server is running on port 8080)..."
wrk -t4 -c100 -d10s http://localhost:8080/

echo ""
echo "Testing JSON API performance..."
wrk -t4 -c100 -d10s http://localhost:8080/api/data

echo "======================================"
echo " Benchmark Complete."
echo "======================================"
