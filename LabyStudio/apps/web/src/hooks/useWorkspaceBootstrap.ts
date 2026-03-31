import { useEffect, useState } from "react";
import type { RuntimeContext } from "@labystudio/shared";
import { api } from "../lib/api";

type UseWorkspaceBootstrapArgs = {
    onContext: (context: RuntimeContext) => void;
    onError: (error: unknown) => void;
};

export function useWorkspaceBootstrap({ onContext, onError }: UseWorkspaceBootstrapArgs): RuntimeContext | null {
    const [context, setContext] = useState<RuntimeContext | null>(null);

    useEffect(() => {
        let cancelled = false;

        api.getContext()
            .then((nextContext) => {
                if (cancelled) {
                    return;
                }

                setContext(nextContext);
                onContext(nextContext);
            })
            .catch((error) => {
                if (!cancelled) {
                    onError(error);
                }
            });

        return () => {
            cancelled = true;
        };
    }, [onContext, onError]);

    return context;
}