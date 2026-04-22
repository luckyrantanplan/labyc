import js from "@eslint/js";
import globals from "globals";
import tseslint from "typescript-eslint";
import { fileURLToPath } from "node:url";

const tsconfigRootDir = fileURLToPath(new URL(".", import.meta.url));
const typedTsConfigs = [
  ...tseslint.configs.strictTypeChecked,
  ...tseslint.configs.stylisticTypeChecked
].map((config) => ({
  ...config,
  files: ["**/*.ts"]
}));

export default tseslint.config(
  {
    ignores: ["dist/**", "coverage/**"]
  },
  js.configs.recommended,
  {
    files: ["eslint.config.mjs"],
    languageOptions: {
      globals: {
        ...globals.node,
        URL: "readonly"
      }
    }
  },
  ...typedTsConfigs,
  {
    files: ["**/*.ts"],
    languageOptions: {
      globals: globals.node,
      parserOptions: {
        project: "./tsconfig.json",
        tsconfigRootDir
      }
    },
    rules: {
      "no-console": "off"
    }
  }
);