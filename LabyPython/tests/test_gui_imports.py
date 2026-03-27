"""
Tests that verify the Python GUI modules can be imported and are compatible
with PyQt6 and modern Python.
"""

import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))


class TestImports:
    """Test that all modules can be imported without errors."""

    def test_import_pyqt6_core(self):
        """PyQt6.QtCore should be importable."""
        from PyQt6 import QtCore
        assert hasattr(QtCore, 'QDir')

    def test_import_pyqt6_widgets(self):
        """PyQt6.QtWidgets should be importable."""
        from PyQt6 import QtWidgets
        assert hasattr(QtWidgets, 'QMainWindow')
        assert hasattr(QtWidgets, 'QApplication')

    def test_import_pyqt6_gui(self):
        """PyQt6.QtGui should be importable."""
        from PyQt6 import QtGui
        assert hasattr(QtGui, 'QAction')

    def test_import_protobuf(self):
        """google.protobuf should be importable."""
        from google.protobuf import json_format
        assert hasattr(json_format, 'MessageToJson')
        assert hasattr(json_format, 'Parse')

    def test_import_allconfig_pb2(self):
        """AllConfig_pb2 should be importable and contain expected classes."""
        from LabyPython import AllConfig_pb2
        assert hasattr(AllConfig_pb2, 'AllConfig')
        assert hasattr(AllConfig_pb2, 'SkeletonGrid')
        assert hasattr(AllConfig_pb2, 'Routing')
        assert hasattr(AllConfig_pb2, 'GraphicRendering')
        assert hasattr(AllConfig_pb2, 'PenStroke')
        assert hasattr(AllConfig_pb2, 'Placement')
        assert hasattr(AllConfig_pb2, 'Cell')
        assert hasattr(AllConfig_pb2, 'RoutingCost')
        assert hasattr(AllConfig_pb2, 'AlternateRouting')
        assert hasattr(AllConfig_pb2, 'Filepaths')

    def test_import_maze_creator(self):
        """mazeCreator UI class should be importable."""
        from LabyPython.mazeCreator import Ui_MainWindow
        assert Ui_MainWindow is not None

    def test_import_watcher(self):
        """watchAndLaunch.Watcher should be importable."""
        from LabyPython.watchAndLaunch import Watcher
        assert Watcher is not None

    def test_import_watchdog(self):
        """watchdog library should be importable."""
        import watchdog
        assert watchdog is not None


class TestQDirFilterConstants:
    """Test that Qt6 enum constants used in App.py are accessible."""

    def test_qdir_filter_no_dot_and_dot_dot(self):
        """QDir.Filter.NoDotAndDotDot should exist in PyQt6."""
        from PyQt6.QtCore import QDir
        assert hasattr(QDir.Filter, 'NoDotAndDotDot')

    def test_qdir_filter_files(self):
        """QDir.Filter.Files should exist in PyQt6."""
        from PyQt6.QtCore import QDir
        assert hasattr(QDir.Filter, 'Files')

    def test_qdir_filter_combination(self):
        """Filter flags should be combinable with |."""
        from PyQt6.QtCore import QDir
        combined = QDir.Filter.NoDotAndDotDot | QDir.Filter.Files
        assert combined is not None


class TestQMessageBoxConstants:
    """Test that QMessageBox constants used in App.py are accessible."""

    def test_standard_button_yes(self):
        """QMessageBox.StandardButton.Yes should exist."""
        from PyQt6.QtWidgets import QMessageBox
        assert hasattr(QMessageBox.StandardButton, 'Yes')

    def test_standard_button_no(self):
        """QMessageBox.StandardButton.No should exist."""
        from PyQt6.QtWidgets import QMessageBox
        assert hasattr(QMessageBox.StandardButton, 'No')

    def test_standard_button_ok(self):
        """QMessageBox.StandardButton.Ok should exist."""
        from PyQt6.QtWidgets import QMessageBox
        assert hasattr(QMessageBox.StandardButton, 'Ok')

    def test_standard_button_combination(self):
        """Standard buttons should be combinable with |."""
        from PyQt6.QtWidgets import QMessageBox
        combined = QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        assert combined is not None
