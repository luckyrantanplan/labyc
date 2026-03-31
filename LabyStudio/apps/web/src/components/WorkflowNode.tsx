import { memo, useEffect, useState, type KeyboardEvent } from "react";
import {
    BROADCAST_INPUT_HANDLE,
    NUMERIC_CONSTANT_OUTPUT_HANDLE,
    OPERATION_LEFT_HANDLE,
    OPERATION_RESULT_HANDLE,
    OPERATION_RIGHT_HANDLE,
    SVG_INPUT_HANDLE,
    SVG_OUTPUT_HANDLE,
    toParameterInputHandle,
    toParameterOutputHandle
} from "@labystudio/shared";
import { Handle, Position, type NodeProps } from "reactflow";
import type { DisplayNodeData } from "../lib/workflowGraph";

type EditableNumberRowProps = {
    label: string;
    value: number;
    step?: string | number;
    disabled?: boolean;
    min?: number;
    max?: number;
    inputHandleId?: string;
    outputHandleId?: string;
    onChange?: (value: number) => void;
};

const workflowControlClassName = "workflow-node__control nodrag nopan nowheel";

function stopCanvasEventPropagation(event: { stopPropagation(): void }): void {
    event.stopPropagation();
}

function formatNumberDraft(value: number): string {
    return `${value}`;
}

function parseNumberDraft(draft: string): number | null {
    const normalized = draft.trim();
    if (normalized === "" || normalized === "-" || normalized === "." || normalized === "-.") {
        return null;
    }

    const parsed = Number(normalized);
    return Number.isFinite(parsed) ? parsed : null;
}

function clampNumberValue(value: number, min?: number, max?: number): number {
    let nextValue = value;
    if (typeof min === "number") {
        nextValue = Math.max(min, nextValue);
    }

    if (typeof max === "number") {
        nextValue = Math.min(max, nextValue);
    }

    return nextValue;
}

type CanvasNumberInputProps = {
    value: number;
    step?: string | number;
    min?: number;
    max?: number;
    disabled?: boolean;
    onChange?: (value: number) => void;
};

function CanvasNumberInput({
    value,
    step,
    min,
    max,
    disabled,
    onChange
}: CanvasNumberInputProps) {
    const [draft, setDraft] = useState(() => formatNumberDraft(value));
    const [isFocused, setIsFocused] = useState(false);

    useEffect(() => {
        if (!isFocused) {
            setDraft(formatNumberDraft(value));
        }
    }, [isFocused, value]);

    const commitDraft = (nextDraft: string, normalizeDraft: boolean): void => {
        setDraft(nextDraft);
        const parsed = parseNumberDraft(nextDraft);
        if (parsed === null) {
            return;
        }

        const normalizedValue = clampNumberValue(parsed, min, max);
        onChange?.(normalizedValue);

        if (normalizeDraft || normalizedValue !== parsed) {
            setDraft(formatNumberDraft(normalizedValue));
        }
    };

    const handleBlur = (): void => {
        setIsFocused(false);
        const parsed = parseNumberDraft(draft);
        if (parsed === null) {
            setDraft(formatNumberDraft(value));
            return;
        }

        const normalizedValue = clampNumberValue(parsed, min, max);
        onChange?.(normalizedValue);
        setDraft(formatNumberDraft(normalizedValue));
    };

    return (
        <input
            type="number"
            className={workflowControlClassName}
            step={step}
            min={min}
            max={max}
            value={draft}
            disabled={disabled}
            draggable={false}
            onFocus={() => { setIsFocused(true); }}
            onBlur={handleBlur}
            onPointerDown={stopCanvasEventPropagation}
            onWheel={stopCanvasEventPropagation}
            onChange={(event) => { commitDraft(event.target.value, false); }}
            onKeyDown={(event: KeyboardEvent<HTMLInputElement>) => {
                if (event.key === "Enter") {
                    event.currentTarget.blur();
                }
            }}
        />
    );
}

function EditableNumberRow({
    label,
    value,
    step = "0.1",
    disabled = false,
    min,
    max,
    inputHandleId,
    outputHandleId,
    onChange
}: EditableNumberRowProps) {
    return (
        <div className="workflow-node__parameter-row">
            {inputHandleId ? (
                <Handle
                    id={inputHandleId}
                    type="target"
                    position={Position.Left}
                    className="workflow-node__handle workflow-node__handle--number"
                    style={{ top: "50%", left: 0, transform: "translate(-50%, -50%)" }}
                />
            ) : null}

            <label className="workflow-node__field nodrag nopan">
                <span>{label}</span>
                <CanvasNumberInput
                    step={step}
                    value={value}
                    min={min}
                    max={max}
                    disabled={disabled}
                    onChange={onChange}
                />
            </label>

            {outputHandleId ? (
                <Handle
                    id={outputHandleId}
                    type="source"
                    position={Position.Right}
                    className="workflow-node__handle workflow-node__handle--number"
                    style={{ top: "50%", right: 0, left: "auto", transform: "translate(50%, -50%)" }}
                />
            ) : null}
        </div>
    );
}

function WorkflowNode({ data, selected }: NodeProps<DisplayNodeData>) {
    if (data.displayType === "transformer") {
        return (
            <div className={`workflow-node workflow-node--transformer ${selected ? "is-selected" : ""}`}>
                <div className="workflow-node__transport-row workflow-node__transport-row--artifact">
                    <Handle
                        id={SVG_INPUT_HANDLE}
                        type="target"
                        position={Position.Left}
                        className="workflow-node__handle workflow-node__handle--svg"
                        style={{ top: "50%", left: 0, transform: "translate(-50%, -50%)" }}
                    />
                    <div>
                        <span className={`workflow-node__badge workflow-node__badge--${data.kind}`}>{data.kind}</span>
                        <div className="workflow-node__transport-copy">Source SVG: {data.sourceSvgPath ? data.sourceSvgPath.split("/").at(-1) : "Not connected"}</div>
                    </div>
                    <div className={`workflow-node__status workflow-node__status--${data.status}`}>{data.status}</div>
                    <Handle
                        id={SVG_OUTPUT_HANDLE}
                        type="source"
                        position={Position.Right}
                        className="workflow-node__handle workflow-node__handle--svg"
                        style={{ top: "50%", right: 0, left: "auto", transform: "translate(50%, -50%)" }}
                    />
                </div>

                <label className="workflow-node__field workflow-node__field--label nodrag nopan">
                    <span>Label</span>
                    <input
                        className={workflowControlClassName}
                        value={data.label}
                        draggable={false}
                        onPointerDown={stopCanvasEventPropagation}
                        onChange={(event) => { data.onUpdateLabel?.(event.target.value); }}
                    />
                </label>

                <div className="workflow-node__artifact-summary">
                    <div>
                        <span>Output</span>
                        <strong>{data.outputSvgPath ? data.outputSvgPath.split("/").at(-1) : "Pending"}</strong>
                    </div>
                    <div>
                        <span>Config</span>
                        <strong>{data.configPath ? data.configPath.split("/").at(-1) : "Generated on run"}</strong>
                    </div>
                </div>

                {typeof data.alternateRoutingEnabled === "boolean" ? (
                    <label className="workflow-node__checkbox-row nodrag nopan">
                        <input
                            className={workflowControlClassName}
                            type="checkbox"
                            checked={data.alternateRoutingEnabled}
                            draggable={false}
                            onPointerDown={stopCanvasEventPropagation}
                            onChange={(event) => { data.onToggleAlternateRouting?.(event.target.checked); }}
                        />
                        <span>Alternate routing</span>
                    </label>
                ) : null}

                <div className="workflow-node__parameter-list">
                    {data.parameterFields.map((field) => (
                        <EditableNumberRow
                            key={field.id}
                            label={field.label}
                            value={field.isConnected ? field.resolvedValue : field.value}
                            step={field.step}
                            disabled={field.isConnected}
                            inputHandleId={toParameterInputHandle(field.id)}
                            outputHandleId={toParameterOutputHandle(field.id)}
                            onChange={(value) => { data.onUpdateParameter?.(field.id, value); }}
                        />
                    ))}
                </div>
            </div>
        );
    }

    if (data.displayType === "numericConstant") {
        return (
            <div className={`workflow-node workflow-node--numeric ${selected ? "is-selected" : ""}`}>
                <div className="workflow-node__header">
                    <span className="workflow-node__badge workflow-node__badge--numeric">constant</span>
                    <input
                        className={`${workflowControlClassName} workflow-node__inline-input`}
                        value={data.label}
                        draggable={false}
                        onPointerDown={stopCanvasEventPropagation}
                        onChange={(event) => { data.onUpdateLabel?.(event.target.value); }}
                    />
                </div>

                <EditableNumberRow
                    label="Value"
                    value={data.value}
                    outputHandleId={NUMERIC_CONSTANT_OUTPUT_HANDLE}
                    onChange={(value) => { data.onUpdateValue?.(value); }}
                />

                <div className="workflow-node__result-row">
                    <span>Output</span>
                    <strong>{data.outputValue}</strong>
                </div>
            </div>
        );
    }

    if (data.displayType === "operation") {
        return (
            <div className={`workflow-node workflow-node--numeric ${selected ? "is-selected" : ""}`}>
                <div className="workflow-node__header workflow-node__header--stacked">
                    <span className="workflow-node__badge workflow-node__badge--numeric">operation</span>
                    <input
                        className={`${workflowControlClassName} workflow-node__inline-input`}
                        value={data.label}
                        draggable={false}
                        onPointerDown={stopCanvasEventPropagation}
                        onChange={(event) => { data.onUpdateLabel?.(event.target.value); }}
                    />
                    <select
                        className={workflowControlClassName}
                        value={data.operation}
                        onPointerDown={stopCanvasEventPropagation}
                        onChange={(event) => { data.onUpdateOperation?.(event.target.value as "add" | "multiply"); }}
                    >
                        <option value="add">Add</option>
                        <option value="multiply">Multiply</option>
                    </select>
                </div>

                <div className="workflow-node__parameter-list">
                    <EditableNumberRow
                        label="Left"
                        value={data.left.isConnected ? data.left.resolvedValue : data.left.value}
                        disabled={data.left.isConnected}
                        inputHandleId={OPERATION_LEFT_HANDLE}
                        onChange={(value) => { data.onUpdateInput?.("left", value); }}
                    />
                    <EditableNumberRow
                        label="Right"
                        value={data.right.isConnected ? data.right.resolvedValue : data.right.value}
                        disabled={data.right.isConnected}
                        inputHandleId={OPERATION_RIGHT_HANDLE}
                        onChange={(value) => { data.onUpdateInput?.("right", value); }}
                    />
                    <div className="workflow-node__result-row workflow-node__result-row--handle">
                        <span>Result</span>
                        <strong>{data.result}</strong>
                        <Handle
                            id={OPERATION_RESULT_HANDLE}
                            type="source"
                            position={Position.Right}
                            className="workflow-node__handle workflow-node__handle--number"
                            style={{ top: "50%", right: 0, left: "auto", transform: "translate(50%, -50%)" }}
                        />
                    </div>
                </div>
            </div>
        );
    }

    if (data.displayType === "broadcast") {
        return (
            <div className={`workflow-node workflow-node--numeric ${selected ? "is-selected" : ""}`}>
                <div className="workflow-node__header workflow-node__header--stacked">
                    <span className="workflow-node__badge workflow-node__badge--numeric">broadcast</span>
                    <input
                        className={`${workflowControlClassName} workflow-node__inline-input`}
                        value={data.label}
                        draggable={false}
                        onPointerDown={stopCanvasEventPropagation}
                        onChange={(event) => { data.onUpdateLabel?.(event.target.value); }}
                    />
                </div>

                <div className="workflow-node__broadcast-controls">
                    <EditableNumberRow
                        label="Input"
                        value={data.inputConnected ? data.inputValue : data.value}
                        disabled={data.inputConnected}
                        inputHandleId={BROADCAST_INPUT_HANDLE}
                        onChange={(value) => { data.onUpdateValue?.(value); }}
                    />
                    <label className="workflow-node__field nodrag nopan">
                        <span>Outputs</span>
                        <CanvasNumberInput
                            min={2}
                            max={8}
                            step={1}
                            value={data.outputs.length}
                            onChange={data.onUpdateOutputs}
                        />
                    </label>
                </div>

                <div className="workflow-node__parameter-list">
                    {data.outputs.map((output, index) => (
                        <div key={output.handleId} className="workflow-node__result-row workflow-node__result-row--handle">
                            <span>{`Output ${index + 1}`}</span>
                            <strong>{output.value}</strong>
                            <Handle
                                id={output.handleId}
                                type="source"
                                position={Position.Right}
                                className="workflow-node__handle workflow-node__handle--number"
                                style={{ top: "50%", right: 0, left: "auto", transform: "translate(50%, -50%)" }}
                            />
                        </div>
                    ))}
                </div>
            </div>
        );
    }

    return null;
}

export default memo(WorkflowNode);
