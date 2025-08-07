import socket
import tests.integration.utils.startup as startup
import struct
import tests.integration.utils.serialization as serialization
import tests.integration.utils.network as network
from hft.serialization.gen.fbs.domain import LoginResponse, OrderStatus

class HftClient:
    def __init__(self):
        self.upstream_sock = None
        self.downstream_sock = None

    def connect(self):
        self.upstream_sock = socket.create_connection((startup.HOST, startup.PORT_UP))
        self.downstream_sock = socket.create_connection((startup.HOST, startup.PORT_DOWN))

    def login(self, user_name, password):
        print(f"Authenticating upstream channel: {user_name} {password}")
        login_msg = serialization.create_login_request(user_name, password)
        self.sendUpstream(login_msg)
        login_response = self.receiveUpstream()

        assert isinstance(login_response, LoginResponse.LoginResponse), f"Unexpected server response to LoginRequest {login_response}"
        assert login_response.Ok(), f"Failed to login: {login_response.Error()}"

        print(f"Authenticating downstream channel: {login_response.Token()}")

        token_bind_request = serialization.create_token_bind_request(login_response.Token())
        self.sendDownstream(token_bind_request)
        token_bind_response = self.receiveDownstream()

        assert isinstance(token_bind_response, LoginResponse.LoginResponse), f"Unexpected server response to TokenBindRequest {token_bind_response}"
        assert token_bind_response.Ok(), f"Failed to login: {token_bind_response.Error()}"

    def sendUpstream(self, message: bytes):
        self.upstream_sock.sendall(message)

    def sendDownstream(self, message: bytes):
        self.downstream_sock.sendall(message)

    def receiveUpstream(self, timeout=0.1):
        message = network.recv_framed_message(self.upstream_sock, timeout)
        return serialization.parse_message(message)
    
    def receiveDownstream(self, timeout=0.1):
        message = network.recv_framed_message(self.downstream_sock, timeout)
        return serialization.parse_message(message)

    def close(self):
        if self.upstream_sock:
            self.upstream_sock.close()
        if self.downstream_sock:
            self.downstream_sock.close()
