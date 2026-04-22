import assert from "node:assert/strict";
import test from "node:test";
import { buildGridConfigPayload, buildRenderConfigPayload, buildRouteConfigPayload } from "../src/payload.js";
import { gridConfigFixture, renderConfigFixture, routeConfigFixture } from "./fixtures.js";

void test("buildGridConfigPayload produces protobuf-shaped JSON", () => {
  const payload = buildGridConfigPayload("/tmp/input.svg", "/tmp/output.svg", gridConfigFixture);

  assert.deepEqual(payload, {
    skeletonGrid: {
      outputfile: "/tmp/output.svg",
      inputfile: "/tmp/input.svg",
      simplificationOfOriginalSVG: 0.1,
      maxSep: 5,
      minSep: 0.1,
      seed: 3
    }
  });
});

void test("buildRouteConfigPayload stringifies maxRoutingAttempt and omits alternateRouting when disabled", () => {
  const payload = buildRouteConfigPayload("/tmp/input.svg", "/tmp/output.svg", routeConfigFixture) as {
    routing: {
      placement: { maxRoutingAttempt: string };
      alternateRouting?: unknown;
    };
  };

  assert.equal(payload.routing.placement.maxRoutingAttempt, "300");
  assert.equal(payload.routing.alternateRouting, undefined);
});

void test("buildRenderConfigPayload preserves pen configuration", () => {
  const payload = buildRenderConfigPayload("/tmp/input.svg", "/tmp/output.svg", renderConfigFixture) as {
    gGraphicRendering: {
      penConfig: { thickness: number };
    };
  };

  assert.equal(payload.gGraphicRendering.penConfig.thickness, 0.25);
});

void test("buildRouteConfigPayload includes alternateRouting when enabled", () => {
  const payload = buildRouteConfigPayload("/tmp/input.svg", "/tmp/output.svg", {
    ...routeConfigFixture,
    enableAlternateRouting: true
  }) as {
    routing: {
      alternateRouting?: { pruning: number };
    };
  };

  assert.equal(payload.routing.alternateRouting?.pruning, 0);
});