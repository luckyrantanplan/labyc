export type Point = {
    x: number;
    y: number;
};

export function clamp(value: number, min: number, max: number): number {
    return Math.min(max, Math.max(min, value));
}

export function fitPreviewToGeometry(container: HTMLDivElement): boolean {
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