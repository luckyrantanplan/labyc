import {
    buildGridConfigPayload,
    buildRenderConfigPayload,
    buildRouteConfigPayload,
    gridConfigSchema,
    renderConfigSchema,
    routeToggleFieldGroup,
    routeConfigSchema,
    transformerParameterDefinitions,
    type GridConfig,
    type NumberFieldConfig,
    type RenderConfig,
    type RouteConfig,
    type TransformerNodeKind,
    type WorkflowNodeData
} from "@labystudio/shared";

export type StageKind = Extract<WorkflowNodeData["kind"], TransformerNodeKind>;

export type StageConfigByKind = {
    grid: GridConfig;
    route: RouteConfig;
    render: RenderConfig;
};

type ToggleFieldGroup<T> = typeof routeToggleFieldGroup & {
    isEnabled: (config: T) => boolean;
    setEnabled: (config: T, enabled: boolean) => T;
    fields: readonly NumberFieldConfig<T>[];
};

type StageInspectorDefinition<K extends StageKind> = {
    fields: readonly NumberFieldConfig<StageConfigByKind[K]>[];
    parse: (value: unknown) => StageConfigByKind[K];
    buildPayload: (inputPath: string, outputPath: string, config: StageConfigByKind[K]) => Record<string, unknown>;
    toggleFieldGroup?: ToggleFieldGroup<StageConfigByKind[K]>;
};

export const stageInspectorDefinitions = {
    grid: {
        fields: transformerParameterDefinitions.grid,
        parse: (value) => gridConfigSchema.parse(value),
        buildPayload: buildGridConfigPayload
    },
    route: {
        fields: transformerParameterDefinitions.route,
        parse: (value) => routeConfigSchema.parse(value),
        buildPayload: buildRouteConfigPayload,
        toggleFieldGroup: routeToggleFieldGroup as ToggleFieldGroup<RouteConfig>
    },
    render: {
        fields: transformerParameterDefinitions.render,
        parse: (value) => renderConfigSchema.parse(value),
        buildPayload: buildRenderConfigPayload
    }
} satisfies {
    grid: StageInspectorDefinition<"grid">;
    route: StageInspectorDefinition<"route">;
    render: StageInspectorDefinition<"render">;
};

export function parseStageConfig(kind: StageKind, value: unknown): GridConfig | RouteConfig | RenderConfig {
    if (kind === "grid") {
        return stageInspectorDefinitions.grid.parse(value);
    }

    if (kind === "route") {
        return stageInspectorDefinitions.route.parse(value);
    }

    return stageInspectorDefinitions.render.parse(value);
}

export function buildStagePayload(kind: StageKind, inputPath: string, outputPath: string, config: GridConfig | RouteConfig | RenderConfig): string {
    if (kind === "grid") {
        return `${JSON.stringify(stageInspectorDefinitions.grid.buildPayload(inputPath, outputPath, config as GridConfig), null, 2)}\n`;
    }

    if (kind === "route") {
        return `${JSON.stringify(stageInspectorDefinitions.route.buildPayload(inputPath, outputPath, config as RouteConfig), null, 2)}\n`;
    }

    return `${JSON.stringify(stageInspectorDefinitions.render.buildPayload(inputPath, outputPath, config as RenderConfig), null, 2)}\n`;
}