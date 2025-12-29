import sys
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[2]
GEN_PATH = ROOT / "common" / "src" / "gen" / "fbs" / "python"

sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(GEN_PATH))

import os
import configparser
import pytest
from tests.integration.utils import db_utils

def pytest_addoption(parser):
    base_dir = os.path.dirname(os.path.abspath(__file__))
    default_config = os.path.join(base_dir, "config", "itest_config.ini")
    parser.addoption(
        "--it-config",
        action="store",
        default=default_config,
        help="Path to integration test ini config file",
    )
    parser.addoption(
        "--stress-iterations", 
        action="store", 
        default="5000000", 
        help="Iterations for stress test"
    )

@pytest.fixture(scope="session")
def config(request):
    path = request.config.getoption("--it-config")
    if not os.path.exists(path):
        pytest.fail(f"Config file not found: {path}")
    parser = configparser.ConfigParser()
    parser.read(path)
    return parser

@pytest.fixture(scope="session")
def stress_iterations(request):
    return int(request.config.getoption("--stress-iterations"))

@pytest.fixture(scope="session")
def tickers():
    tickers_list = db_utils.read_tickers()
    assert tickers_list, "Tickers list is empty or None"
    print(f"Loaded {len(tickers_list)} tickers from db")
    return tickers_list

@pytest.fixture(scope="session")
def client_name(config):
    return config.get("credentials", "name")

@pytest.fixture(scope="session")
def client_password(config):
    return config.get("credentials", "password")
