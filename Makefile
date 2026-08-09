.PHONY: all build clean test e2e-test benchmark run

all: build

build:
	mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)

clean:
	rm -rf build

test: build
	cd build && ctest --output-on-failure

e2e-test: build
	python3 tests/integration_tests.py

benchmark: build
	bash benchmarks/run_benchmarks.sh

run: build
	./build/basic_server --port 3000
