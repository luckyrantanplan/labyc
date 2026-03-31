import { useCallback, useEffect, useRef, useState, type MouseEvent as ReactMouseEvent, type PointerEvent as ReactPointerEvent, type WheelEvent as ReactWheelEvent } from "react";
import { clamp, type Point } from "../lib/svgPreviewGeometry";

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

export function useSvgViewport(previewAvailable: boolean) {
    const viewportRef = useRef<HTMLDivElement | null>(null);
    const dragRef = useRef<DragState | null>(null);
    const [isOpen, setIsOpen] = useState(false);
    const [zoom, setZoom] = useState(1);
    const [pan, setPan] = useState({ x: 0, y: 0 });

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
        return () => { window.removeEventListener("keydown", handleKeyDown); };
    }, [isOpen]);

    const closePreview = useCallback(() => {
        dragRef.current = null;
        setIsOpen(false);
    }, []);

    const resetView = useCallback(() => {
        setPan({ x: 0, y: 0 });
        setZoom(1);
    }, []);

    const applyZoom = useCallback((nextZoom: number, pointer?: Point) => {
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
    }, [pan.x, pan.y, zoom]);

    const openPreview = useCallback((event: ReactMouseEvent<HTMLButtonElement>) => {
        event.stopPropagation();
        if (!previewAvailable) {
            return;
        }

        setPan({ x: 0, y: 0 });
        setZoom(1);
        setIsOpen(true);
    }, [previewAvailable]);

    const zoomOut = useCallback(() => {
        applyZoom(zoom / ZOOM_STEP);
    }, [applyZoom, zoom]);

    const zoomIn = useCallback(() => {
        applyZoom(zoom * ZOOM_STEP);
    }, [applyZoom, zoom]);

    const handleWheel = useCallback((event: ReactWheelEvent<HTMLDivElement>) => {
        event.preventDefault();
        event.stopPropagation();
        applyZoom(event.deltaY < 0 ? zoom * ZOOM_STEP : zoom / ZOOM_STEP, {
            x: event.clientX,
            y: event.clientY
        });
    }, [applyZoom, zoom]);

    const handlePointerDown = useCallback((event: ReactPointerEvent<HTMLDivElement>) => {
        if (!previewAvailable || event.button !== 0) {
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
    }, [pan.x, pan.y, previewAvailable]);

    const handlePointerMove = useCallback((event: ReactPointerEvent<HTMLDivElement>) => {
        const drag = dragRef.current;
        if (drag?.pointerId !== event.pointerId) {
            return;
        }

        setPan({
            x: drag.originX + (event.clientX - drag.startX),
            y: drag.originY + (event.clientY - drag.startY)
        });
    }, []);

    const handlePointerUp = useCallback((event: ReactPointerEvent<HTMLDivElement>) => {
        if (dragRef.current?.pointerId === event.pointerId) {
            dragRef.current = null;
            viewportRef.current?.releasePointerCapture(event.pointerId);
        }
    }, []);

    return {
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
    };
}