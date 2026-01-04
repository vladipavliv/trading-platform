import struct
import socket

def recv_framed_message(sock: socket.socket, timeout: float = 0.1) -> bytes:
    sock.settimeout(timeout)
    try:
        size_bytes = b""
        while len(size_bytes) < 2:
            chunk = sock.recv(2 - len(size_bytes))
            if not chunk:
                raise ConnectionError("Socket closed while reading size header")
            size_bytes += chunk

        size = struct.unpack('<H', size_bytes)[0]

        payload = b""
        while len(payload) < size:
            chunk = sock.recv(size - len(payload))
            if not chunk:
                raise ConnectionError("Socket closed while reading payload")
            payload += chunk

        return payload

    except socket.timeout:
        raise TimeoutError("Receiving data timed out")

    finally:
        sock.settimeout(None)
