import { useEffect, useState } from "react";
import type { DirectoryEntry } from "@labystudio/shared";
import { api } from "../lib/api";

type UseDirectoryListingArgs = {
    browserDir: string;
    onError: (error: unknown) => void;
};

export function useDirectoryListing({ browserDir, onError }: UseDirectoryListingArgs): DirectoryEntry[] {
    const [browserEntries, setBrowserEntries] = useState<DirectoryEntry[]>([]);

    useEffect(() => {
        if (!browserDir) {
            setBrowserEntries([]);
            return;
        }

        let cancelled = false;

        api.listDirectory(browserDir)
            .then((listing) => {
                if (!cancelled) {
                    setBrowserEntries(listing.entries);
                }
            })
            .catch((error: unknown) => {
                if (!cancelled) {
                    onError(error);
                }
            });

        return () => {
            cancelled = true;
        };
    }, [browserDir, onError]);

    return browserEntries;
}