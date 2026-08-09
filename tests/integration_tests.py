import time
import subprocess
import urllib.request
import urllib.error
import json
import socket
import sys

print("==========================================")
print("       Orbit E2E Integration Tests        ")
print("==========================================")

server_process = subprocess.Popen(["./build/basic_server", "--port", "3000"])
time.sleep(2) # Wait for startup

if server_process.poll() is not None:
    print("Server failed to start.")
    sys.exit(1)

base_url = "http://127.0.0.1:3000"
successes = 0
failures = 0

def assert_eq(expected, actual, test_name):
    global successes, failures
    if expected == actual:
        print(f"[PASS] {test_name}")
        successes += 1
    else:
        print(f"[FAIL] {test_name} - Expected: {expected}, Got: {actual}")
        failures += 1

def request(method, path, data=None, headers={}):
    req = urllib.request.Request(base_url + path, method=method, headers=headers)
    if data:
        req.data = data.encode('utf-8')
    try:
        with urllib.request.urlopen(req) as response:
            return response.status, response.read().decode('utf-8'), dict(response.headers)
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode('utf-8'), dict(e.headers)

# Tests
try:
    # 1. Simple GET
    status, body, _ = request("GET", "/api/v1/users")
    assert_eq(200, status, "GET /api/v1/users returns 200")
    assert_eq("List of users", body, "GET /api/v1/users body check")

    # 2. JSON POST
    status, body, _ = request("POST", "/api/v1/users", data='{"name":"Integration"}', headers={"Content-Type": "application/json"})
    assert_eq(201, status, "POST /api/v1/users returns 201")
    assert_eq("User created: Integration", body, "POST /api/v1/users parsing check")

    # 3. Dynamic route
    status, body, _ = request("GET", "/api/v1/users/42")
    assert_eq(200, status, "GET /api/v1/users/42 returns 200")
    assert_eq("User ID: 42", body, "GET /api/v1/users/42 parameter extraction")

    # 4. Session cookie injection
    status, body, headers = request("GET", "/api/v1/users")
    has_cookie = "Set-Cookie" in headers and "session_id=" in headers["Set-Cookie"]
    assert_eq(True, has_cookie, "SessionManager injects session_id cookie")

    # 5. Rate Limiting (Make 105 requests rapidly)
    rate_limited = False
    for i in range(110):
        s, b, _ = request("GET", "/api/v1/users")
        if s == 429:
            rate_limited = True
            break
    assert_eq(True, rate_limited, "Rate Limiter successfully triggered 429")

except Exception as e:
    print(f"[ERROR] Test suite crashed: {e}")
    failures += 1

print("\nCleaning up...")
server_process.terminate()
server_process.wait()

print(f"\nResults: {successes} Passed, {failures} Failed")
if failures > 0:
    sys.exit(1)
