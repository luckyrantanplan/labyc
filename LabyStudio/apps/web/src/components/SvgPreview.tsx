import { memo, useEffect, useLayoutEffect, useRef, useState } from "react";
import { createPortal } from "react-dom";
import DOMPurify from "dompurify";
import { useSvgViewport } from "../hooks/useSvgViewport";
import { api } from "../lib/api";
import { fitPreviewToGeometry } from "../lib/svgPreviewGeometry";

type SvgPreviewProps = {
    path?: string;
    cacheKey?: string;
    title: string;
    compact?: boolean;
};

function SvgPreview({ path, cacheKey, title, compact = false }: SvgPreviewProps) {
    const previewRef = useRef<HTMLDivElement | null>(null);
    const [previewSvg, setPreviewSvg] = useState("");
    const [hasVisibleGeometry, setHasVisibleGeometry] = useState(true);
    const {
        viewportRef,
        isOpen,
        zoom,
        pan,
        openPreview,
        closePreview,
        resetView,
        zoomOut,
        zoomIn,
        handleWheel,
        handlePointerDown,
        handlePointerMove,
        handlePointerUp
    } = useSvgViewport(Boolean(previewSvg));

    useEffect(() => {
        if (!path) {
            setPreviewSvg("");
            setHasVisibleGeometry(true);
            return;
        }

        const controller = new AbortController();
        const timeoutId = window.setTimeout(() => { controller.abort(); }, 5000);

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
                    <div className="svg-preview-overlay__panel" onClick={(event) => { event.stopPropagation(); }}>
                        <div className="svg-preview-overlay__toolbar">
                            <div>
                                <div className="svg-preview-overlay__eyebrow">SVG preview</div>
                                <strong>{title}</strong>
                            </div>
                            <div className="svg-preview-overlay__actions">
                                <button type="button" onClick={zoomOut}>-</button>
                                <button type="button" onClick={zoomIn}>+</button>
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