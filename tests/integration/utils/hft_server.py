import socket
import tests.integration.utils.startup as startup

class HftServer:
    def __init__(self):
        self.upstream_sock = None
        self.downstream_sock = None

    def connect(self):
        self.upstream_sock = socket.create_connection((startup.HOST, startup.PORT_UP))
        self.downstream_sock = socket.create_connection((startup.HOST, startup.PORT_DOWN))

    def send(self, message: bytes):
        self.upstream_sock.sendall(message)

    def receive(self, buffer_size=4096):
        return self.downstream_sock.recv(buffer_size)

    def close(self):
        if self.upstream_sock:
            self.upstream_sock.close()
        if self.downstream_sock:
            self.downstream_sock.close()
