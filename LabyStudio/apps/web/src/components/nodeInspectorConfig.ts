import {
    buildGridConfigPayload,
    buildRenderConfigPayload,
    buildRouteConfigPayload,
    gridConfigSchema,
    renderConfigSchema,
    routeConfigSchema,
    type GridConfig,
    type RenderConfig,
    type RouteConfig,
    type WorkflowNodeData
} from "@labystudio/shared";

export type StageKind = Exclude<WorkflowNodeData["kind"], "source">;

export type StageConfigByKind = {
    grid: GridConfig;
    route: RouteConfig;
    render: RenderConfig;
};

export type NumberFieldConfig<T> = {
    label: string;
    step?: string;
    getValue: (config: T) => number;
    setValue: (config: T, value: number) => T;
};

type ToggleFieldGroup<T> = {
    label: string;
    isEnabled: (config: T) => boolean;
    setEnabled: (config: T, enabled: boolean) => T;
    fields: NumberFieldConfig<T>[];
};

type StageInspectorDefinition<K extends StageKind> = {
    fields: NumberFieldConfig<StageConfigByKind[K]>[];
    parse: (value: unknown) => StageConfigByKind[K];
    buildPayload: (inputPath: string, outputPath: string, config: StageConfigByKind[K]) => Record<string, unknown>;
    toggleFieldGroup?: ToggleFieldGroup<StageConfigByKind[K]>;
};

const gridFields: NumberFieldConfig<GridConfig>[] = [
    {
        label: "Simplification",
        step: "0.1",
        getValue: (config) => config.simplificationOfOriginalSVG,
        setValue: (config, value) => ({ ...config, simplificationOfOriginalSVG: value })
    },
    {
        label: "Max separation",
        step: "0.1",
        getValue: (config) => config.maxSep,
        setValue: (config, value) => ({ ...config, maxSep: value })
    },
    {
        label: "Min separation",
        step: "0.1",
        getValue: (config) => config.minSep,
        setValue: (config, value) => ({ ...config, minSep: value })
    },
    {
        label: "Seed",
        getValue: (config) => config.seed,
        setValue: (config, value) => ({ ...config, seed: value })
    }
];

const routeFields: NumberFieldConfig<RouteConfig>[] = [
    {
        label: "Initial thickness",
        step: "0.1",
        getValue: (config) => config.initialThickness,
        setValue: (config, value) => ({ ...config, initialThickness: value })
    },
    {
        label: "Decrement factor",
        step: "0.1",
        getValue: (config) => config.decrementFactor,
        setValue: (config, value) => ({ ...config, decrementFactor: value })
    },
    {
        label: "Minimal thickness",
        step: "0.1",
        getValue: (config) => config.minimalThickness,
        setValue: (config, value) => ({ ...config, minimalThickness: value })
    },
    {
        label: "Smoothing tension",
        step: "0.1",
        getValue: (config) => config.smoothingTension,
        setValue: (config, value) => ({ ...config, smoothingTension: value })
    },
    {
        label: "Smoothing iteration",
        getValue: (config) => config.smoothingIteration,
        setValue: (config, value) => ({ ...config, smoothingIteration: value })
    },
    {
        label: "Max routing attempt",
        getValue: (config) => config.maxRoutingAttempt,
        setValue: (config, value) => ({ ...config, maxRoutingAttempt: value })
    },
    {
        label: "Routing seed",
        getValue: (config) => config.routing.seed,
        setValue: (config, value) => ({ ...config, routing: { ...config.routing, seed: value } })
    },
    {
        label: "Routing max random",
        getValue: (config) => config.routing.maxRandom,
        setValue: (config, value) => ({ ...config, routing: { ...config.routing, maxRandom: value } })
    },
    {
        label: "Distance unit cost",
        getValue: (config) => config.routing.distanceUnitCost,
        setValue: (config, value) => ({ ...config, routing: { ...config.routing, distanceUnitCost: value } })
    },
    {
        label: "Via unit cost",
        getValue: (config) => config.routing.viaUnitCost,
        setValue: (config, value) => ({ ...config, routing: { ...config.routing, viaUnitCost: value } })
    },
    {
        label: "Cell seed",
        getValue: (config) => config.cell.seed,
        setValue: (config, value) => ({ ...config, cell: { ...config.cell, seed: value } })
    },
    {
        label: "Cell max pin",
        getValue: (config) => config.cell.maxPin,
        setValue: (config, value) => ({ ...config, cell: { ...config.cell, maxPin: value } })
    },
    {
        label: "Cell start net",
        getValue: (config) => config.cell.startNet,
        setValue: (config, value) => ({ ...config, cell: { ...config.cell, startNet: value } })
    },
    {
        label: "Cell resolution",
        step: "0.1",
        getValue: (config) => config.cell.resolution,
        setValue: (config, value) => ({ ...config, cell: { ...config.cell, resolution: value } })
    }
];

const alternateRouteFields: NumberFieldConfig<RouteConfig>[] = [
    {
        label: "Max thickness",
        step: "0.1",
        getValue: (config) => config.alternateRouting.maxThickness,
        setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, maxThickness: value } })
    },
    {
        label: "Min thickness",
        step: "0.1",
        getValue: (config) => config.alternateRouting.minThickness,
        setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, minThickness: value } })
    },
    {
        label: "Pruning",
        getValue: (config) => config.alternateRouting.pruning,
        setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, pruning: value } })
    },
    {
        label: "Thickness percent",
        step: "0.1",
        getValue: (config) => config.alternateRouting.thicknessPercent,
        setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, thicknessPercent: value } })
    },
    {
        label: "Simplify distance",
        step: "0.1",
        getValue: (config) => config.alternateRouting.simplifyDist,
        setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, simplifyDist: value } })
    }
];

const renderFields: NumberFieldConfig<RenderConfig>[] = [
    {
        label: "Smoothing tension",
        step: "0.1",
        getValue: (config) => config.smoothingTension,
        setValue: (config, value) => ({ ...config, smoothingTension: value })
    },
    {
        label: "Smoothing iterations",
        step: "0.1",
        getValue: (config) => config.smoothingIterations,
        setValue: (config, value) => ({ ...config, smoothingIterations: value })
    },
    {
        label: "Pen thickness",
        step: "0.05",
        getValue: (config) => config.penConfig.thickness,
        setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, thickness: value } })
    },
    {
        label: "Antisymmetric amplitude",
        step: "0.1",
        getValue: (config) => config.penConfig.antisymmetricAmplitude,
        setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, antisymmetricAmplitude: value } })
    },
    {
        label: "Antisymmetric frequency",
        step: "0.1",
        getValue: (config) => config.penConfig.antisymmetricFreq,
        setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, antisymmetricFreq: value } })
    },
    {
        label: "Antisymmetric seed",
        step: "0.1",
        getValue: (config) => config.penConfig.antisymmetricSeed,
        setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, antisymmetricSeed: value } })
    },
    {
        label: "Symmetric amplitude",
        step: "0.1",
        getValue: (config) => config.penConfig.symmetricAmplitude,
        setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, symmetricAmplitude: value } })
    },
    {
        label: "Symmetric frequency",
        step: "0.1",
        getValue: (config) => config.penConfig.symmetricFreq,
        setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, symmetricFreq: value } })
    },
    {
        label: "Symmetric seed",
        step: "0.1",
        getValue: (config) => config.penConfig.symmetricSeed,
        setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, symmetricSeed: value } })
    },
    {
        label: "Resolution",
        step: "0.1",
        getValue: (config) => config.penConfig.resolution,
        setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, resolution: value } })
    }
];

export const stageInspectorDefinitions = {
    grid: {
        fields: gridFields,
        parse: (value) => gridConfigSchema.parse(value),
        buildPayload: buildGridConfigPayload
    },
    route: {
        fields: routeFields,
        parse: (value) => routeConfigSchema.parse(value),
        buildPayload: buildRouteConfigPayload,
        toggleFieldGroup: {
            label: "Enable alternate routing",
            isEnabled: (config) => config.enableAlternateRouting,
            setEnabled: (config, enabled) => ({ ...config, enableAlternateRouting: enabled }),
            fields: alternateRouteFields
        }
    },
    render: {
        fields: renderFields,
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