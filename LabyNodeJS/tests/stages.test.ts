import assert from "node:assert/strict";
import test from "node:test";
import {
  grid,
  noise,
  render,
  route,
  source,
  streamline,
} from "../src/stages.js";
import {
  gridConfigFixture,
  noiseConfigFixture,
  renderConfigFixture,
  routeConfigFixture,
  streamLineConfigFixture,
} from "./fixtures.js";

void test("source omits label when none is provided", () => {
  const stage = source("/tmp/input.svg");

  assert.equal("label" in stage, false);
});

void test("route stores the explicit config unchanged", () => {
  const basePlacement = routeConfigFixture.placement;
  const baseAlternateRouting = routeConfigFixture.alternateRouting;
  assert.ok(basePlacement);
  assert.ok(baseAlternateRouting);

  const config = {
    ...routeConfigFixture,
    placement: {
      ...basePlacement,
      routing: {
        ...basePlacement.routing,
        seed: 99,
      },
      cell: {
        ...basePlacement.cell,
        maxPin: 12,
      },
    },
    alternateRouting: {
      ...baseAlternateRouting,
      pruning: 5,
    },
  };
  const stage = route(config, { label: "Route" });
  const placement = stage.config.placement;
  const alternateRouting = stage.config.alternateRouting;
  assert.ok(placement);
  assert.ok(alternateRouting);

  assert.equal(stage.label, "Route");
  assert.equal(placement.routing.seed, 99);
  assert.equal(placement.routing.maxRandom, 300);
  assert.equal(placement.cell.maxPin, 12);
  assert.equal(placement.cell.startNet, 30);
  assert.equal(alternateRouting.pruning, 5);
  assert.equal(alternateRouting.maxThickness, 1.8);
});

void test("render stores the explicit config unchanged", () => {
  const stage = render({
    ...renderConfigFixture,
    penConfig: {
      ...renderConfigFixture.penConfig,
      thickness: 0.9,
    },
  });

  assert.equal(stage.config.penConfig.thickness, 0.9);
  assert.equal(stage.config.penConfig.symmetricFreq, 3);
});

void test("grid stores explicit config without adding a label", () => {
  const stage = grid({
    ...gridConfigFixture,
    seed: 17,
  });

  assert.equal(stage.config.seed, 17);
  assert.equal("label" in stage, false);
});

void test("noise stores explicit config and label", () => {
  const stage = noise(noiseConfigFixture, { label: "Noise" });

  assert.equal(stage.kind, "noise");
  assert.equal(stage.label, "Noise");
  assert.equal(stage.config.previewMode, "ARROWS");
});

void test("streamline stores explicit config unchanged", () => {
  const stage = streamline({
    ...streamLineConfigFixture,
    divisor: 0.6,
  });

  assert.equal(stage.kind, "streamline");
  assert.equal(stage.config.divisor, 0.6);
});
