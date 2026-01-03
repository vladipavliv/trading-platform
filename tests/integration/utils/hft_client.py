import socket
import errno
import time
import select
import tests.integration.utils.startup as startup
import tests.integration.utils.serialization as serialization
import tests.integration.utils.network as network
from hft.serialization.gen.fbs.domain import LoginResponse

class HftClient:
    def __init__(self):
        self.upstream_sock = None
        self.downstream_sock = None

    def connect(self):
        self.upstream_sock = socket.create_connection((startup.HOST, startup.PORT_UP))
        self.downstream_sock = socket.create_connection((startup.HOST, startup.PORT_DOWN))

        self.downstream_sock.setblocking(False)
        self.upstream_sock.setblocking(False)

        self.upstream_sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 10 * 1024 * 1024)
        self.upstream_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 10 * 1024 * 1024)

    def login(self, user_name, password):
        print(f"Authenticating upstream channel: {user_name}")
        login_msg = serialization.create_login_request(user_name, password)
        self.sendUpstream(login_msg, "login_msg")
        
        time.sleep(0.1)

        login_response = self.receiveUpstream(timeout=2.0)

        assert isinstance(login_response, LoginResponse.LoginResponse), f"Unexpected response: {login_response}"
        assert login_response.Ok(), f"Failed to login: {login_response.Error()}"

        print(f"Authenticating downstream channel with token: {login_response.Token()}")

        token_bind_request = serialization.create_token_bind_request(login_response.Token())
        self.sendDownstream(token_bind_request, "token_bind_request")

        time.sleep(0.1)

        token_bind_response = self.receiveDownstream(timeout=2.0)

        assert isinstance(token_bind_response, LoginResponse.LoginResponse), f"Unexpected response: {token_bind_response}"
        assert token_bind_response.Ok(), f"Failed to login: {token_bind_response.Error()}"

    def _non_blocking_send(self, sock, message: bytes, topic: str):
        total_sent = 0
        view = memoryview(message)
        start_time = time.time()
        
        while total_sent < len(message):
            remaining_time = 5.0 - (time.time() - start_time)
            if remaining_time <= 0:
                raise TimeoutError(f"Could not send {topic} after 5s")
            
            # Wait until socket is ready for writing
            _, ready_to_write, in_error = select.select([], [sock], [sock], remaining_time)
            
            if in_error:
                raise ConnectionError(f"Socket error during {topic}")
            
            if not ready_to_write:
                raise TimeoutError(f"Could not send {topic} after 5s")
            
            try:
                sent = sock.send(view[total_sent:])
                if sent > 0:
                    total_sent += sent
            except (BlockingIOError, InterruptedError):
                # Should not happen after select, but be defensive
                time.sleep(0.001)
            except socket.error as e:
                raise ConnectionError(f"Socket error during {topic}: {e}")

    def sendUpstream(self, message: bytes, topic: str = ""):
        self._non_blocking_send(self.upstream_sock, message, topic)

    def sendDownstream(self, message: bytes, topic: str = ""):
        self._non_blocking_send(self.downstream_sock, message, topic)

    def receiveUpstream(self, timeout=0.1):
        message = network.recv_framed_message(self.upstream_sock, timeout)
        return serialization.parse_message(message)
    
    def receiveDownstream(self, timeout=0.1):
        message = network.recv_framed_message(self.downstream_sock, timeout)
        return serialization.parse_message(message)

    def drainDownstreamAndForget(self):
        """Purges all pending data from the downstream socket without parsing."""
        try:
            while True:
                chunk = self.downstream_sock.recv(65536)
                if not chunk: 
                    break
        except BlockingIOError:
            pass
        except socket.error as e:
            if e.errno != errno.EAGAIN and e.errno != errno.EWOULDBLOCK:
                raise e

    def close(self):
        if self.upstream_sock:
            self.upstream_sock.close()
        if self.downstream_sock:
            self.downstream_sock.close()
