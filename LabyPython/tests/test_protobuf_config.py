"""
Tests for protobuf AllConfig message creation, serialization, and parsing.
"""

import sys
import os
import json
import tempfile

import pytest

# Add the source directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src'))

from LabyPython import AllConfig_pb2
from google.protobuf import json_format


class TestAllConfigProtobuf:
    """Tests for AllConfig protobuf message handling."""

    def test_create_empty_config(self):
        """An empty AllConfig message should be created without errors."""
        msg = AllConfig_pb2.AllConfig()
        assert msg is not None

    def test_skeleton_grid_fields(self):
        """SkeletonGrid fields should be settable and retrievable."""
        msg = AllConfig_pb2.AllConfig()
        msg.skeletonGrid.outputfile = "output.svg"
        msg.skeletonGrid.inputfile = "input.svg"
        msg.skeletonGrid.simplificationOfOriginalSVG = 0.1
        msg.skeletonGrid.max_sep = 5.0
        msg.skeletonGrid.min_sep = 0.5
        msg.skeletonGrid.seed = 42

        assert msg.skeletonGrid.outputfile == "output.svg"
        assert msg.skeletonGrid.inputfile == "input.svg"
        assert msg.skeletonGrid.simplificationOfOriginalSVG == pytest.approx(0.1)
        assert msg.skeletonGrid.max_sep == pytest.approx(5.0)
        assert msg.skeletonGrid.min_sep == pytest.approx(0.5)
        assert msg.skeletonGrid.seed == 42

    def test_routing_placement_fields(self):
        """Routing placement fields should be settable."""
        msg = AllConfig_pb2.AllConfig()
        msg.routing.placement.initial_thickness = 1.8
        msg.routing.placement.decrement_factor = 1.5
        msg.routing.placement.minimal_thickness = 0.5
        msg.routing.placement.smoothing_tension = 1.0
        msg.routing.placement.smoothing_iteration = 3
        msg.routing.placement.max_routing_attempt = 300
        msg.routing.placement.cell.seed = 1
        msg.routing.placement.cell.maxPin = 400
        msg.routing.placement.cell.startNet = 30

        assert msg.routing.placement.initial_thickness == pytest.approx(1.8)
        assert msg.routing.placement.cell.maxPin == 400

    def test_routing_cost_fields(self):
        """RoutingCost fields should be settable."""
        msg = AllConfig_pb2.AllConfig()
        msg.routing.placement.routing.seed = 5
        msg.routing.placement.routing.max_random = 300
        msg.routing.placement.routing.distance_unit_cost = 1
        msg.routing.placement.routing.via_unit_cost = 10

        assert msg.routing.placement.routing.seed == 5
        assert msg.routing.placement.routing.via_unit_cost == 10

    def test_graphic_rendering_fields(self):
        """GraphicRendering fields should be settable."""
        msg = AllConfig_pb2.AllConfig()
        msg.gGraphicRendering.outputfile = "render.svg"
        msg.gGraphicRendering.inputfile = "input.svg"
        msg.gGraphicRendering.smoothing_tension = 0.5
        msg.gGraphicRendering.smoothing_iterations = 3.0

        assert msg.gGraphicRendering.outputfile == "render.svg"
        assert msg.gGraphicRendering.smoothing_tension == pytest.approx(0.5)

    def test_pen_stroke_config(self):
        """PenStroke nested message should be accessible."""
        msg = AllConfig_pb2.AllConfig()
        msg.gGraphicRendering.penConfig.thickness = 0.25
        msg.gGraphicRendering.penConfig.antisymmetric_amplitude = 0.3
        msg.gGraphicRendering.penConfig.resolution = 1.0

        assert msg.gGraphicRendering.penConfig.thickness == pytest.approx(0.25)
        assert msg.gGraphicRendering.penConfig.resolution == pytest.approx(1.0)

    def test_alternate_routing_fields(self):
        """AlternateRouting fields should be settable."""
        msg = AllConfig_pb2.AllConfig()
        msg.routing.alternateRouting.maxThickness = 10.0
        msg.routing.alternateRouting.minThickness = 1.0
        msg.routing.alternateRouting.pruning = 5
        msg.routing.alternateRouting.thicknessPercent = 0.8
        msg.routing.alternateRouting.simplifyDist = 0.5

        assert msg.routing.alternateRouting.maxThickness == pytest.approx(10.0)
        assert msg.routing.alternateRouting.pruning == 5

    def test_filepaths_fields(self):
        """Filepaths nested message fields should be settable."""
        msg = AllConfig_pb2.AllConfig()
        msg.routing.filepaths.outputfile = "/path/to/output.svg"
        msg.routing.filepaths.inputfile = "/path/to/input.svg"

        assert msg.routing.filepaths.outputfile == "/path/to/output.svg"
        assert msg.routing.filepaths.inputfile == "/path/to/input.svg"

    def test_json_serialization(self):
        """Config should serialize to JSON and back without data loss."""
        msg = AllConfig_pb2.AllConfig()
        msg.skeletonGrid.outputfile = "test.svg"
        msg.skeletonGrid.seed = 42
        msg.routing.placement.initial_thickness = 1.8

        json_str = json_format.MessageToJson(msg)
        parsed = json.loads(json_str)

        assert "skeletonGrid" in parsed
        assert parsed["skeletonGrid"]["outputfile"] == "test.svg"

    def test_json_round_trip(self):
        """Serializing to JSON and parsing back should preserve all fields."""
        original = AllConfig_pb2.AllConfig()
        original.skeletonGrid.outputfile = "output.svg"
        original.skeletonGrid.inputfile = "input.svg"
        original.skeletonGrid.simplificationOfOriginalSVG = 0.1
        original.skeletonGrid.seed = 42
        original.routing.placement.initial_thickness = 1.8
        original.gGraphicRendering.penConfig.thickness = 0.25

        json_str = json_format.MessageToJson(original)

        restored = AllConfig_pb2.AllConfig()
        json_format.Parse(json_str, restored)

        assert restored.skeletonGrid.outputfile == "output.svg"
        assert restored.skeletonGrid.seed == 42
        assert restored.routing.placement.initial_thickness == pytest.approx(1.8)
        assert restored.gGraphicRendering.penConfig.thickness == pytest.approx(0.25)

    def test_json_file_round_trip(self):
        """Writing config to a file and reading back should work."""
        msg = AllConfig_pb2.AllConfig()
        msg.skeletonGrid.outputfile = "test.svg"
        msg.skeletonGrid.seed = 7

        json_str = json_format.MessageToJson(msg)

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            f.write(json_str)
            f.flush()
            temp_path = f.name

        try:
            with open(temp_path, 'r') as f:
                restored = AllConfig_pb2.AllConfig()
                json_format.Parse(f.read(), restored)
                assert restored.skeletonGrid.outputfile == "test.svg"
                assert restored.skeletonGrid.seed == 7
        finally:
            os.unlink(temp_path)

    def test_binary_serialization(self):
        """Config should serialize to binary and back."""
        original = AllConfig_pb2.AllConfig()
        original.skeletonGrid.seed = 99
        original.routing.placement.initial_thickness = 2.5

        binary = original.SerializeToString()
        assert len(binary) > 0

        restored = AllConfig_pb2.AllConfig()
        restored.ParseFromString(binary)
        assert restored.skeletonGrid.seed == 99
        assert restored.routing.placement.initial_thickness == pytest.approx(2.5)

    def test_default_values(self):
        """Default values for numeric fields should be zero."""
        msg = AllConfig_pb2.AllConfig()
        assert msg.skeletonGrid.seed == 0
        assert msg.skeletonGrid.max_sep == pytest.approx(0.0)
        assert msg.routing.placement.initial_thickness == pytest.approx(0.0)
