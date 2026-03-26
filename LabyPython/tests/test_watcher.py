"""
Tests for the watchAndLaunch worker queue module.
"""

import sys
import os
import time
import threading

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

from LabyPython.watchAndLaunch import Watcher


class TestWatcher:
    """Tests for the Watcher background worker queue."""

    def test_create_watcher(self):
        """Creating a Watcher should start worker threads."""
        watcher = Watcher()
        assert watcher.num_worker_threads == 7
        assert len(watcher.threads) == 7
        for t in watcher.threads:
            assert t.is_alive()
        watcher.stop()

    def test_add_work_executes(self):
        """Work items added to the queue should be executed."""
        watcher = Watcher()
        result = []

        watcher.addWork(lambda: result.append(42))
        time.sleep(0.5)  # Give worker time to execute

        assert 42 in result
        watcher.stop()

    def test_multiple_work_items(self):
        """Multiple work items should all be executed."""
        watcher = Watcher()
        result = []

        for i in range(10):
            watcher.addWork(lambda i=i: result.append(i))

        time.sleep(1.0)  # Give workers time to execute

        assert sorted(result) == list(range(10))
        watcher.stop()

    def test_stop_completes_pending_work(self):
        """stop() should wait for pending work to complete."""
        watcher = Watcher()
        result = []

        for i in range(5):
            watcher.addWork(lambda i=i: result.append(i))

        watcher.stop()

        assert sorted(result) == list(range(5))

    def test_stop_terminates_threads(self):
        """After stop(), all worker threads should be terminated."""
        watcher = Watcher()
        watcher.stop()

        for t in watcher.threads:
            assert not t.is_alive()

    def test_work_order_eventual_consistency(self):
        """All submitted work items should eventually complete."""
        watcher = Watcher()
        counter = {"value": 0}
        lock = threading.Lock()

        def increment():
            with lock:
                counter["value"] += 1

        for _ in range(100):
            watcher.addWork(increment)

        watcher.stop()
        assert counter["value"] == 100
