import socket
import struct
import time

def recv_framed_message(sock: socket.socket, timeout: float = 2.0) -> bytes:
    deadline = time.time() + timeout
    
    def read_exactly(n):
        chunks = []
        bytes_recvd = 0
        while bytes_recvd < n:
            if time.time() > deadline:
                raise TimeoutError(f"Read timed out. Got {bytes_recvd}/{n} bytes")
            try:
                chunk = sock.recv(n - bytes_recvd)
                if not chunk:
                    raise ConnectionError("Server closed connection")
                chunks.append(chunk)
                bytes_recvd += len(chunk)
            except (BlockingIOError, socket.error):
                time.sleep(0.001) # Yield to CPU
                continue
        return b"".join(chunks)

    # Read Header (2 bytes)
    size_bytes = read_exactly(2)
    size = struct.unpack('<H', size_bytes)[0]

    # Read Payload
    return read_exactly(size)