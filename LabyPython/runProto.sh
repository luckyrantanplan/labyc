#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
proto_dir="${script_dir}/../LabyPath/API"
cpp_out="${script_dir}/../LabyPath/src/protoc"
python_out="${script_dir}/src/LabyPython"

mkdir -p "${cpp_out}" "${python_out}"

protoc \
	-I="${proto_dir}" \
	--cpp_out="${cpp_out}" \
	--python_out="${python_out}" \
	"${proto_dir}/AllConfig.proto"
