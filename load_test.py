import socket
import threading
import time

def worker(host, port, num_requests):
    try:
        for _ in range(num_requests):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((host, port))
            req = b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            s.sendall(req)
            data = s.recv(4096)
            s.close()
    except Exception as e:
        print("Error:", e)

def main():
    print("Starting load test...")
    threads = []
    for _ in range(100):
        t = threading.Thread(target=worker, args=('127.0.0.1', 8080, 20))
        t.start()
        threads.append(t)
        
    start = time.time()
    for t in threads:
        t.join()
    print(f"Finished in {time.time() - start:.2f} seconds")

if __name__ == "__main__":
    main()
