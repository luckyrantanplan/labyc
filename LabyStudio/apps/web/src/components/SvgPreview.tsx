import { memo, useEffect, useLayoutEffect, useRef, useState, type PointerEvent as ReactPointerEvent, type WheelEvent as ReactWheelEvent } from "react";
import { createPortal } from "react-dom";
import DOMPurify from "dompurify";
import { api } from "../lib/api";

type SvgPreviewProps = {
    path?: string;
    cacheKey?: string;
    title: string;
    compact?: boolean;
};

type Point = {
    x: number;
    y: number;
};

type DragState = {
    pointerId: number;
    startX: number;
    startY: number;
    originX: number;
    originY: number;
};

const MIN_ZOOM = 0.6;
const MAX_ZOOM = 8;
const ZOOM_STEP = 1.12;

function clamp(value: number, min: number, max: number): number {
    return Math.min(max, Math.max(min, value));
}

function fitPreviewToGeometry(container: HTMLDivElement): boolean {
    const svg = container.querySelector("svg");
    if (!(svg instanceof SVGSVGElement)) {
        return false;
    }

    svg.setAttribute("width", "100%");
    svg.setAttribute("height", "100%");
    svg.setAttribute("preserveAspectRatio", "xMidYMid meet");
    svg.style.overflow = "visible";

    const graphics = svg.querySelectorAll<SVGGraphicsElement>("path, rect, circle, ellipse, line, polyline, polygon, text, use");
    let minX = Number.POSITIVE_INFINITY;
    let minY = Number.POSITIVE_INFINITY;
    let maxX = Number.NEGATIVE_INFINITY;
    let maxY = Number.NEGATIVE_INFINITY;

    for (const graphic of graphics) {
        if (graphic.closest("defs, clipPath, mask, marker, pattern, symbol")) {
            continue;
        }

        const stroke = graphic.getAttribute("stroke");
        if (stroke && stroke !== "none") {
            graphic.setAttribute("vector-effect", "non-scaling-stroke");

            const strokeWidth = Number.parseFloat(graphic.getAttribute("stroke-width") ?? "");
            if (Number.isFinite(strokeWidth) && strokeWidth > 0 && strokeWidth < 0.75) {
                graphic.setAttribute("stroke-width", "0.75");
            }
        }

        try {
            const box = graphic.getBBox();
            if (box.width <= 0 && box.height <= 0) {
                continue;
            }

            minX = Math.min(minX, box.x);
            minY = Math.min(minY, box.y);
            maxX = Math.max(maxX, box.x + Math.max(box.width, 0));
            maxY = Math.max(maxY, box.y + Math.max(box.height, 0));
        } catch {
            continue;
        }
    }

    if (!Number.isFinite(minX) || !Number.isFinite(minY) || !Number.isFinite(maxX) || !Number.isFinite(maxY)) {
        return false;
    }

    const width = Math.max(maxX - minX, 1);
    const height = Math.max(maxY - minY, 1);
    const padding = Math.max(width, height) * 0.08;
    svg.setAttribute("viewBox", `${minX - padding} ${minY - padding} ${width + padding * 2} ${height + padding * 2}`);
    return true;
}

function SvgPreview({ path, cacheKey, title, compact = false }: SvgPreviewProps) {
    const previewRef = useRef<HTMLDivElement | null>(null);
    const viewportRef = useRef<HTMLDivElement | null>(null);
    const dragRef = useRef<DragState | null>(null);
    const [previewSvg, setPreviewSvg] = useState("");
    const [hasVisibleGeometry, setHasVisibleGeometry] = useState(true);
    const [isOpen, setIsOpen] = useState(false);
    const [zoom, setZoom] = useState(1);
    const [pan, setPan] = useState<Point>({ x: 0, y: 0 });

    useEffect(() => {
        if (!path) {
            setPreviewSvg("");
            setHasVisibleGeometry(true);
            return;
        }

        const controller = new AbortController();
        const timeoutId = window.setTimeout(() => controller.abort(), 5000);

        fetch(`${api.svgUrl(path)}&v=${encodeURIComponent(cacheKey ?? path)}`, {
            signal: controller.signal
        })
            .then(async (response) => {
                if (!response.ok) {
                    throw new Error(await response.text());
                }

                return response.text();
            })
            .then((svg) => {
                setHasVisibleGeometry(true);
                setPreviewSvg(DOMPurify.sanitize(svg, { USE_PROFILES: { svg: true, svgFilters: true } }));
            })
            .catch(() => {
                if (!controller.signal.aborted) {
                    setPreviewSvg("");
                    setHasVisibleGeometry(true);
                }
            });

        return () => {
            window.clearTimeout(timeoutId);
            controller.abort();
        };
    }, [cacheKey, path]);

    useLayoutEffect(() => {
        if (!previewSvg || !previewRef.current) {
            return;
        }

        let cancelled = false;

        const applyPreviewFit = () => {
            if (!previewRef.current || cancelled) {
                return;
            }

            const hasGeometry = fitPreviewToGeometry(previewRef.current);
            setHasVisibleGeometry(hasGeometry);

            const normalizedSvg = previewRef.current.innerHTML;
            if (normalizedSvg && normalizedSvg !== previewSvg) {
                setPreviewSvg(normalizedSvg);
            }
        };

        applyPreviewFit();

        const frameId = window.requestAnimationFrame(applyPreviewFit);
        const timeoutId = window.setTimeout(applyPreviewFit, 60);

        return () => {
            cancelled = true;
            window.cancelAnimationFrame(frameId);
            window.clearTimeout(timeoutId);
        };
    }, [previewSvg]);

    useEffect(() => {
        if (!isOpen) {
            return;
        }

        const handleKeyDown = (event: KeyboardEvent) => {
            if (event.key === "Escape") {
                setIsOpen(false);
            }
        };

        window.addEventListener("keydown", handleKeyDown);
        return () => window.removeEventListener("keydown", handleKeyDown);
    }, [isOpen]);

    function openPreview(event: React.MouseEvent<HTMLButtonElement>): void {
        event.stopPropagation();
        if (!previewSvg) {
            return;
        }

        setPan({ x: 0, y: 0 });
        setZoom(1);
        setIsOpen(true);
    }

    function closePreview(): void {
        dragRef.current = null;
        setIsOpen(false);
    }

    function resetView(): void {
        setPan({ x: 0, y: 0 });
        setZoom(1);
    }

    function applyZoom(nextZoom: number, pointer?: Point): void {
        const normalizedZoom = clamp(nextZoom, MIN_ZOOM, MAX_ZOOM);

        if (!pointer || !viewportRef.current) {
            setZoom(normalizedZoom);
            return;
        }

        const rect = viewportRef.current.getBoundingClientRect();
        const relativeX = pointer.x - rect.left - rect.width / 2;
        const relativeY = pointer.y - rect.top - rect.height / 2;
        const contentX = (relativeX - pan.x) / zoom;
        const contentY = (relativeY - pan.y) / zoom;
        setPan({
            x: relativeX - contentX * normalizedZoom,
            y: relativeY - contentY * normalizedZoom
        });
        setZoom(normalizedZoom);
    }

    function handleWheel(event: ReactWheelEvent<HTMLDivElement>): void {
        event.preventDefault();
        event.stopPropagation();
        applyZoom(event.deltaY < 0 ? zoom * ZOOM_STEP : zoom / ZOOM_STEP, {
            x: event.clientX,
            y: event.clientY
        });
    }

    function handlePointerDown(event: ReactPointerEvent<HTMLDivElement>): void {
        if (!previewSvg || event.button !== 0) {
            return;
        }

        event.preventDefault();
        event.stopPropagation();

        dragRef.current = {
            pointerId: event.pointerId,
            startX: event.clientX,
            startY: event.clientY,
            originX: pan.x,
            originY: pan.y
        };

        viewportRef.current?.setPointerCapture(event.pointerId);
    }

    function handlePointerMove(event: ReactPointerEvent<HTMLDivElement>): void {
        const drag = dragRef.current;
        if (!drag || drag.pointerId !== event.pointerId) {
            return;
        }

        setPan({
            x: drag.originX + (event.clientX - drag.startX),
            y: drag.originY + (event.clientY - drag.startY)
        });
    }

    function handlePointerUp(event: ReactPointerEvent<HTMLDivElement>): void {
        if (dragRef.current?.pointerId === event.pointerId) {
            dragRef.current = null;
            viewportRef.current?.releasePointerCapture(event.pointerId);
        }
    }

    return (
        <>
            <button
                type="button"
                className={`workflow-node__preview-frame${compact ? " workflow-node__preview-frame--compact" : ""} workflow-node__preview-button${previewSvg ? " is-clickable" : ""}`}
                onClick={openPreview}
            >
                {previewSvg ? (
                    <>
                        <div
                            ref={previewRef}
                            className={`workflow-node__preview-svg${compact ? " workflow-node__preview-svg--compact" : ""}`}
                            dangerouslySetInnerHTML={{ __html: previewSvg }}
                        />
                        <div className="workflow-node__preview-hint">Open preview</div>
                        {!hasVisibleGeometry ? (
                            <div className="workflow-node__preview-note">No visible geometry in this SVG output.</div>
                        ) : null}
                    </>
                ) : (
                    <div className="workflow-node__preview-empty">Preview appears here once the SVG exists.</div>
                )}
            </button>

            {isOpen ? createPortal(
                <div className="svg-preview-overlay" onClick={closePreview}>
                    <div className="svg-preview-overlay__panel" onClick={(event) => event.stopPropagation()}>
                        <div className="svg-preview-overlay__toolbar">
                            <div>
                                <div className="svg-preview-overlay__eyebrow">SVG preview</div>
                                <strong>{title}</strong>
                            </div>
                            <div className="svg-preview-overlay__actions">
                                <button type="button" onClick={() => applyZoom(zoom / ZOOM_STEP)}>-</button>
                                <button type="button" onClick={() => applyZoom(zoom * ZOOM_STEP)}>+</button>
                                <button type="button" onClick={resetView}>Reset</button>
                                <button type="button" onClick={closePreview}>Close</button>
                            </div>
                        </div>
                        <div
                            ref={viewportRef}
                            className="svg-preview-overlay__viewport"
                            onWheel={handleWheel}
                            onPointerDown={handlePointerDown}
                            onPointerMove={handlePointerMove}
                            onPointerUp={handlePointerUp}
                            onPointerCancel={handlePointerUp}
                        >
                            <div
                                className="svg-preview-overlay__canvas"
                                style={{ transform: `translate(-50%, -50%) translate(${pan.x}px, ${pan.y}px) scale(${zoom})` }}
                            >
                                <div className="svg-preview-overlay__graphic" dangerouslySetInnerHTML={{ __html: previewSvg }} />
                            </div>
                            {!hasVisibleGeometry ? (
                                <div className="svg-preview-overlay__note">No visible geometry in this SVG output.</div>
                            ) : null}
                        </div>
                    </div>
                </div>,
                document.body
            ) : null}
        </>
    );
}

export default memo(SvgPreview);