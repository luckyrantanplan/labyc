#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat <<'EOF'
Usage: ./run-clang-tidy-all.sh [options]

Runs clang-tidy on all C++ translation units in LabyPath/src and
LabyPath/tests using the compile database in .cmake/build.

Options:
  --configure           Force a CMake configure before running clang-tidy.
  --fix                 Apply clang-tidy fixes in place.
  --build-dir PATH      Override the build directory. Default: .cmake/build
  --log-dir PATH        Override the log directory. Default: .logs/clang-tidy
  --help                Show this help text.
EOF
}

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
BUILD_DIR="${ROOT_DIR}/.cmake/build"
LOG_DIR="${ROOT_DIR}/.logs/clang-tidy"
FORCE_CONFIGURE=0
APPLY_FIXES=0
MAX_PARALLEL=3

while [[ $# -gt 0 ]]; do
    case "$1" in
        --configure)
            FORCE_CONFIGURE=1
            ;;
        --fix)
            APPLY_FIXES=1
            ;;
        --build-dir)
            shift
            [[ $# -gt 0 ]] || {
                echo "Missing value for --build-dir" >&2
                exit 2
            }
            BUILD_DIR="$1"
            ;;
        --log-dir)
            shift
            [[ $# -gt 0 ]] || {
                echo "Missing value for --log-dir" >&2
                exit 2
            }
            LOG_DIR="$1"
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

require_command() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Required command not found: $1" >&2
        exit 127
    }
}

configure_if_needed() {
    local compile_commands="${BUILD_DIR}/compile_commands.json"
    if [[ ${FORCE_CONFIGURE} -eq 1 || ! -f "${compile_commands}" ]]; then
        mkdir -p "${BUILD_DIR}"
        cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
            -DBUILD_TESTING=ON \
            -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_CXX_COMPILER=g++-14 \
            -DLABYPATH_BUILD_TESTS=ON \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    fi

    [[ -f "${compile_commands}" ]] || {
        echo "Missing compile database: ${compile_commands}" >&2
        exit 1
    }
}

require_command cmake
require_command clang-tidy
require_command find
require_command mktemp
require_command sort

configure_if_needed

mkdir -p "${LOG_DIR}"

timestamp=$(date +%Y%m%d-%H%M%S)
mode_name="check"
if [[ ${APPLY_FIXES} -eq 1 ]]; then
    mode_name="fix"
fi

log_file="${LOG_DIR}/${mode_name}-${timestamp}.log"
failure_file="${LOG_DIR}/${mode_name}-failures-${timestamp}.txt"
touch "${log_file}" "${failure_file}"

mapfile -t files < <(
    find "${ROOT_DIR}/LabyPath/src" "${ROOT_DIR}/LabyPath/tests" -type f \
        \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \) | sort
)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No translation units found under LabyPath/src or LabyPath/tests" >&2
    exit 1
fi

failed_count=0
processed_count=0
temp_dir=$(mktemp -d "${LOG_DIR}/.${mode_name}-${timestamp}.XXXXXX")

cleanup() {
    rm -rf -- "${temp_dir}"
}

run_clang_tidy_job() {
    local file="$1"
    local output_file="$2"
    local relative_file=${file#"${ROOT_DIR}/"}
    local tidy_cmd=(clang-tidy -p "${BUILD_DIR}")

    if [[ ${APPLY_FIXES} -eq 1 ]]; then
        tidy_cmd+=(-fix)
    fi
    tidy_cmd+=("${file}")

    {
        printf '\n== %s ==\n' "${relative_file}"
        "${tidy_cmd[@]}"
    } >"${output_file}" 2>&1
}

wait_for_batch() {
    local index
    local pid
    local status

    for index in "${!batch_pids[@]}"; do
        pid=${batch_pids[$index]}
        status=0
        if ! wait "${pid}"; then
            status=$?
        fi

        cat -- "${batch_logs[$index]}" | tee -a "${log_file}"

        if [[ ${status} -ne 0 ]]; then
            echo "${batch_files[$index]}" | tee -a "${failure_file}"
            failed_count=$((failed_count + 1))
        fi
    done

    batch_pids=()
    batch_files=()
    batch_logs=()
}

trap cleanup EXIT

batch_pids=()
batch_files=()
batch_logs=()

for file in "${files[@]}"; do
    processed_count=$((processed_count + 1))
    relative_file=${file#"${ROOT_DIR}/"}

    batch_log="${temp_dir}/$(printf '%05d' "${processed_count}").log"
    run_clang_tidy_job "${file}" "${batch_log}" &

    batch_pids+=("$!")
    batch_files+=("${relative_file}")
    batch_logs+=("${batch_log}")

    if [[ ${#batch_pids[@]} -eq ${MAX_PARALLEL} ]]; then
        wait_for_batch
    fi
done

if [[ ${#batch_pids[@]} -gt 0 ]]; then
    wait_for_batch
fi

echo
echo "Processed files: ${processed_count}"
echo "Failures: ${failed_count}"
echo "Log file: ${log_file}"
echo "Failure list: ${failure_file}"

if [[ ${failed_count} -ne 0 ]]; then
    exit 1
fi