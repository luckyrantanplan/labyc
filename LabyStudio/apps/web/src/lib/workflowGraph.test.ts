import { describe, expect, it } from "vitest";
import type { Edge } from "reactflow";
import { createFlowNode, buildDisplayGraph, toDisplayNode, toGraphDocument } from "./workflowGraph";

describe("workflowGraph", () => {
    it("builds display nodes and edges for workflow stages and artifacts", () => {
        const source = createFlowNode("source", "source", 60, 200);
        if (source.data.kind !== "source") {
            throw new Error("Expected a source node");
        }

        source.data.sourcePath = "/tmp/source.svg";
        const grid = createFlowNode("grid", "grid", 340, 200);
        const edges: Edge[] = [{ id: "edge-source-grid", source: "source", target: "grid" }];
        const nodes = [source, grid];

        const logicalNodes = nodes.map((node) => toDisplayNode(node, nodes, edges));
        const displayGraph = buildDisplayGraph(logicalNodes, edges);

        expect(displayGraph.nodes.find((node) => node.id === "source")?.data.displayType).toBe("artifact");
        expect(displayGraph.nodes.find((node) => node.id === "grid")?.data.displayType).toBe("transformer");
        expect(displayGraph.nodes.find((node) => node.id === "grid::config")).toBeDefined();
        expect(displayGraph.nodes.find((node) => node.id === "grid::final-svg")).toBeDefined();
        expect(displayGraph.edges.some((edge) => edge.id === "edge-source-grid::source-svg")).toBe(true);
    });

    it("converts workflow nodes and edges into a graph document", () => {
        const source = createFlowNode("source", "source", 60, 200);
        if (source.data.kind !== "source") {
            throw new Error("Expected a source node");
        }

        source.data.sourcePath = "/tmp/source.svg";
        const render = createFlowNode("render", "render", 900, 200);
        const graph = toGraphDocument(
            [source, render],
            [{ id: "edge-source-render", source: "source", target: "render" }]
        );

        expect(graph.version).toBe(1);
        expect(graph.nodes).toHaveLength(2);
        expect(graph.edges).toEqual([{ id: "edge-source-render", source: "source", target: "render" }]);
    });
});