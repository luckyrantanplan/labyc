FROM ubuntu:24.04 AS builder

LABEL maintainer="LabyPath Project"
LABEL description="Build environment for the LabyPath C++ and Python projects"

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# ─── Install build tools and dependencies ────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Compiler and build tools
    g++-14 \
    gcc-14 \
    cmake \
    ninja-build \
    pkg-config \
    # CGAL and its dependencies
    libcgal-dev \
    libgmp-dev \
    libmpfr-dev \
    # Boost
    libboost-all-dev \
    # Protocol Buffers
    libprotobuf-dev \
    protobuf-compiler \
    # utilities
    ripgrep \
    # FFTW3
    libfftw3-dev \
    # SVG++ and rapidxml_ns
    libsvgpp-dev \
    # Microsoft GSL (Guidelines Support Library)
    libmsgsl-dev \
    # Google Test
    libgtest-dev \
    # Python (for GUI and tests)
    python3 \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Set GCC 14 as the default compiler
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100

# ─── Copy project sources ───────────────────────────────────────────────────
WORKDIR /app
COPY LabyPath/ /app/LabyPath/
COPY LabyPython/ /app/LabyPython/

# ─── Build the C++ project ──────────────────────────────────────────────────
WORKDIR /app/LabyPath
RUN cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-14 \
    -DLABYPATH_BUILD_TESTS=ON \
    && cmake --build build --parallel "$(nproc)"

# ─── Run C++ tests ───────────────────────────────────────────────────────────
RUN cd build && ctest --output-on-failure

# ─── Install Python dependencies and run Python tests ────────────────────────
WORKDIR /app/LabyPython
RUN python3 -m venv /app/venv \
    && /app/venv/bin/pip install --no-cache-dir protobuf watchdog pytest \
    && /app/venv/bin/python -m pytest tests/ -v --ignore=tests/test_gui_imports.py

# ─── Runtime image ───────────────────────────────────────────────────────────
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libgmp10 \
    libmpfr6 \
    libprotobuf-lite32t64 \
    libfftw3-double3 \
    libgomp1 \
    python3 \
    python3-venv \
    ripgrep \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/LabyPath/build/labypath /usr/local/bin/labypath
COPY --from=builder /app/LabyPython/ /app/LabyPython/
COPY --from=builder /app/venv/ /app/venv/

ENTRYPOINT ["labypath"]
