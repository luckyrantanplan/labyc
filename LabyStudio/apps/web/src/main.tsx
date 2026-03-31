import ReactDOM from "react-dom/client";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import "reactflow/dist/style.css";
import App from "./App";
import "./styles.css";

const queryClient = new QueryClient({
    defaultOptions: {
        queries: {
            retry: 3,
            refetchOnWindowFocus: true
        },
        mutations: {
            retry: 0
        }
    }
});

const rootElement = document.getElementById("root");

if (!rootElement) {
    throw new Error("Missing root element with id \"root\".");
}

ReactDOM.createRoot(rootElement).render(
    <QueryClientProvider client={queryClient}>
        <App />
    </QueryClientProvider>
);