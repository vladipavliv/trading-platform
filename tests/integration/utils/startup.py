import os
import sys
import time
import socket
import subprocess
import pathlib
import subprocess
from contextlib import contextmanager
from threading import Thread

ROOT = pathlib.Path(__file__).resolve().parents[3]
GEN_PATH = ROOT / "common" / "src" / "gen" / "fbs" / "python"

BINARIES = {
    "server": (ROOT / "build" / "server" / "hft_server",
               ROOT / "tests" / "integration" / "config" / "itest_server_config.ini"),
    "client": (ROOT / "build" / "client" / "hft_client",
               ROOT / "tests" / "integration" / "config" / "itest_client_config.ini"),
    "monitor": (ROOT / "build" / "monitor" / "hft_monitor",
                ROOT / "tests" / "integration" / "config" / "itest_monitor_config.ini"),
}

HOST = '127.0.0.1'
PORT_UP = 8080
PORT_DOWN = 8081
PORT_UDP = 8082

def _start_process(name, log_to_console=True):
    bin_path, cfg_path = BINARIES[name]
    print(f"Starting {name}")

    stdout_target = None if log_to_console else subprocess.DEVNULL
    stderr_target = None if log_to_console else subprocess.DEVNULL

    proc = subprocess.Popen(
        [str(bin_path), "--config", str(cfg_path)],
        stdin=subprocess.PIPE,
        stdout=stdout_target,
        stderr=stderr_target,
        text=True
    )

    for _ in range(10):
        if proc.poll() is not None:
            out, err = proc.communicate(timeout=0.1)
            raise RuntimeError(
                f"{name.capitalize()} failed to start.\nstdout:\n{out}\nstderr:\n{err}"
            )
        time.sleep(0.1)

    return proc

def stop(proc):
    try:
        proc.stdin.write('q\n')
        proc.stdin.flush()
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        print(f"Process failed to stop, killing it")
        proc.kill()
        proc.wait()
    except Exception as e:
        print(f"Error during stopping process: {e}")
        proc.kill()
        proc.wait()

    if proc.poll() is None:
        raise RuntimeError("Failed to stop the process.")


def print_stream(stream, prefix):
    for line in iter(stream.readline, ''):
        print(f"{prefix}: {line.rstrip()}")
    stream.close()

def is_server_running(process_name="hft_server"):
    pids = [pid for pid in os.listdir('/proc') if pid.isdigit()]
    for pid in pids:
        try:
            # 1. Check the short name (comm)
            with open(os.path.join('/proc', pid, 'comm'), 'r') as f:
                if f.read().strip() == process_name:
                    return True
            
            # 2. Check the full command line (in case it's prefixed by gdb)
            with open(os.path.join('/proc', pid, 'cmdline'), 'r') as f:
                # cmdline uses null bytes (\0) as delimiters
                cmdline = f.read().replace('\0', ' ')
                if process_name in cmdline:
                    return True
                    
        except (IOError, OSError):
            continue
    return False

@contextmanager
def running_process(name, log_to_console=True):
    if name == "server" and is_server_running():
        print(f"Server is already running. Skipping startup.")
        try:
            yield None
        finally:
            print(f"Server was already running; leaving it alive.")
            return

    proc = _start_process(name, log_to_console)

    stdout_thread = None
    stderr_thread = None

    if proc.stdout is not None:
        stdout_thread = Thread(target=print_stream, args=(proc.stdout, f"{name.upper()} STDOUT"), daemon=True)
        stdout_thread.start()

    if proc.stderr is not None:
        stderr_thread = Thread(target=print_stream, args=(proc.stderr, f"{name.upper()} STDERR"), daemon=True)
        stderr_thread.start()

    try:
        yield proc
    finally:
        stop(proc)
        if stdout_thread is not None:
            stdout_thread.join()
        if stderr_thread is not None:
            stderr_thread.join()

running_server = lambda: running_process("server")
running_client = lambda: running_process("client")
running_monitor = lambda: running_process("monitor", log_to_console=False)
