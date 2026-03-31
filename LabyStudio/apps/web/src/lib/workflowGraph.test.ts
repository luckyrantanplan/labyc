import { describe, expect, it } from "vitest";
import { toParameterInputHandle } from "@labystudio/shared";
import type { Edge } from "reactflow";
import { createFlowNode, buildDisplayGraph, toDisplayNode, toGraphDocument } from "./workflowGraph";

describe("workflowGraph", () => {
    it("builds direct display nodes for source, transformer, and parameter wiring", () => {
        const source = createFlowNode("source", "source", 60, 200);
        if (source.data.kind !== "source") {
            throw new Error("Expected a source node");
        }

        source.data.sourcePath = "/tmp/source.svg";
        const grid = createFlowNode("grid", "grid", 340, 200);
        const constant = createFlowNode("numericConstant", "constant", 220, 420);
        if (constant.data.kind !== "numericConstant") {
            throw new Error("Expected a numeric constant node");
        }

        constant.data.value = 9;
        const edges: Edge[] = [
            { id: "edge-source-grid", source: "source", target: "grid", data: { kind: "artifact" } },
            {
                id: "edge-constant-grid-seed",
                source: "constant",
                target: "grid",
                sourceHandle: "value",
                targetHandle: toParameterInputHandle("seed"),
                data: { kind: "parameter" }
            }
        ];
        const nodes = [source, grid, constant];

        const logicalNodes = nodes.map((node) => toDisplayNode(node, nodes, edges));
        const displayGraph = buildDisplayGraph(logicalNodes, edges);
        const gridDisplayNode = displayGraph.nodes.find((node) => node.id === "grid");

        if (gridDisplayNode?.data.displayType !== "transformer") {
            throw new Error("Expected a transformer display node");
        }

        expect(displayGraph.nodes.find((node) => node.id === "source")?.data.displayType).toBe("artifact");
        expect(displayGraph.nodes.find((node) => node.id === "grid")?.data.displayType).toBe("transformer");
        expect(displayGraph.nodes.find((node) => node.id === "constant")?.data.displayType).toBe("numericConstant");
        expect(gridDisplayNode.data.parameterFields.find((field) => field.id === "seed")).toMatchObject({
            isConnected: true,
            resolvedValue: 9
        });
        expect(displayGraph.edges).toHaveLength(2);
    });

    it("converts workflow nodes and mixed edges into a graph document", () => {
        const source = createFlowNode("source", "source", 60, 200);
        if (source.data.kind !== "source") {
            throw new Error("Expected a source node");
        }

        source.data.sourcePath = "/tmp/source.svg";
        const render = createFlowNode("render", "render", 900, 200);
        const constant = createFlowNode("numericConstant", "constant", 420, 360);
        const graph = toGraphDocument(
            [source, render, constant],
            [
                { id: "edge-source-render", source: "source", target: "render", data: { kind: "artifact" } },
                {
                    id: "edge-constant-render-tension",
                    source: "constant",
                    target: "render",
                    sourceHandle: "value",
                    targetHandle: toParameterInputHandle("smoothingTension"),
                    data: { kind: "parameter" }
                }
            ]
        );

        expect(graph.version).toBe(1);
        expect(graph.nodes).toHaveLength(3);
        expect(graph.edges).toEqual([
            { id: "edge-source-render", source: "source", target: "render", kind: "artifact", sourceHandle: undefined, targetHandle: undefined },
            {
                id: "edge-constant-render-tension",
                source: "constant",
                target: "render",
                kind: "parameter",
                sourceHandle: "value",
                targetHandle: toParameterInputHandle("smoothingTension")
            }
        ]);
    });
});