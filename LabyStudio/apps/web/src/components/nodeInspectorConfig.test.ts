import { createDefaultNodeData } from "@labystudio/shared";
import { describe, expect, it } from "vitest";
import { buildStagePayload, parseStageConfig } from "./nodeInspectorConfig";

describe("nodeInspectorConfig", () => {
    it("parses a grid config through the shared schema", () => {
        const gridNode = createDefaultNodeData("grid");
        if (gridNode.kind !== "grid") {
            throw new Error("Expected a grid node");
        }

        const parsed = parseStageConfig("grid", gridNode.config);
        expect(parsed).toEqual(gridNode.config);
    });

    it("builds route payloads and includes alternate routing only when enabled", () => {
        const routeNode = createDefaultNodeData("route");
        if (routeNode.kind !== "route") {
            throw new Error("Expected a route node");
        }

        const withoutAlternateRouting = JSON.parse(buildStagePayload(
            "route",
            "/tmp/input.svg",
            "/tmp/output.svg",
            routeNode.config
        )) as { routing: { alternateRouting?: unknown } };
        expect(withoutAlternateRouting.routing.alternateRouting).toBeUndefined();

        const withAlternateRouting = JSON.parse(buildStagePayload(
            "route",
            "/tmp/input.svg",
            "/tmp/output.svg",
            {
                ...routeNode.config,
                enableAlternateRouting: true
            }
        )) as { routing: { alternateRouting?: unknown } };
        expect(withAlternateRouting.routing.alternateRouting).toBeDefined();
    });
});