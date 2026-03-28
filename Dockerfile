FROM ubuntu:24.04 AS builder

ARG CGAL_VERSION=6.1.1
ARG CGAL_LIBRARY_SHA256=37e9fffe48a83209b070e1914c6aa0a7bae8076749712ab78b53245e176e0e0e
ARG CGAL_LIBRARY_URL=https://github.com/CGAL/cgal/releases/download/v${CGAL_VERSION}/CGAL-${CGAL_VERSION}-library.tar.xz
ARG PROTOBUF_VERSION=34.1
ARG PROTOBUF_SHA256=e4e6ff10760cf747a2decd1867741f561b216bd60cc4038c87564713a6da1848
ARG PROTOBUF_URL=https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOBUF_VERSION}/protobuf-${PROTOBUF_VERSION}.tar.gz

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
    ca-certificates \
    curl \
    git \
    xz-utils \
    # CGAL and its dependencies
    libgmp-dev \
    libmpfr-dev \
    # Boost
    libboost-all-dev \
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

# Install a pinned CGAL release because Ubuntu 24.04's libcgal-dev does not track 6.1.1.
RUN set -eux; \
    curl -fsSL "${CGAL_LIBRARY_URL}" -o /tmp/cgal.tar.xz; \
    echo "${CGAL_LIBRARY_SHA256}  /tmp/cgal.tar.xz" | sha256sum -c -; \
    tar -xf /tmp/cgal.tar.xz -C /tmp; \
    CGAL_SRC_DIR="$(find /tmp -maxdepth 1 -mindepth 1 -type d -name 'CGAL-*' | head -n 1)"; \
    cmake -S "${CGAL_SRC_DIR}" -B /tmp/cgal-build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DWITH_examples=OFF \
    -DWITH_demos=OFF; \
    cmake --build /tmp/cgal-build --target install --parallel "$(nproc)"; \
    rm -rf /tmp/cgal.tar.xz "${CGAL_SRC_DIR}" /tmp/cgal-build

# Install a pinned Protobuf release because Ubuntu 24.04 ships an older 3.21.x package.
RUN set -eux; \
    curl -fsSL "${PROTOBUF_URL}" -o /tmp/protobuf.tar.gz; \
    echo "${PROTOBUF_SHA256}  /tmp/protobuf.tar.gz" | sha256sum -c -; \
    tar -xzf /tmp/protobuf.tar.gz -C /tmp; \
    PROTOBUF_SRC_DIR="$(find /tmp -maxdepth 1 -mindepth 1 -type d -name 'protobuf-*' | head -n 1)"; \
    cmake -S "${PROTOBUF_SRC_DIR}" -B /tmp/protobuf-build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_CXX_STANDARD=20 \
    -Dprotobuf_BUILD_TESTS=OFF; \
    cmake --build /tmp/protobuf-build --target install --parallel "$(nproc)"; \
    rm -rf /tmp/protobuf.tar.gz "${PROTOBUF_SRC_DIR}" /tmp/protobuf-build

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
    && /app/venv/bin/pip install --no-cache-dir protobuf==7.34.1 watchdog pytest \
    && /app/venv/bin/python -m pytest tests/ -v --ignore=tests/test_gui_imports.py

# ─── Runtime image ───────────────────────────────────────────────────────────
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libgmp10 \
    libmpfr6 \
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
