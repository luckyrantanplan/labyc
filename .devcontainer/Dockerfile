FROM ubuntu:24.04

ARG CGAL_VERSION=6.1.1
ARG CGAL_LIBRARY_SHA256=37e9fffe48a83209b070e1914c6aa0a7bae8076749712ab78b53245e176e0e0e
ARG CGAL_LIBRARY_URL=https://github.com/CGAL/cgal/releases/download/v${CGAL_VERSION}/CGAL-${CGAL_VERSION}-library.tar.xz
ARG PROTOBUF_VERSION=34.1
ARG PROTOBUF_SHA256=e4e6ff10760cf747a2decd1867741f561b216bd60cc4038c87564713a6da1848
ARG PROTOBUF_URL=https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOBUF_VERSION}/protobuf-${PROTOBUF_VERSION}.tar.gz

LABEL maintainer="LabyPath Project"
LABEL description="Development container for LabyPath (C++ / Python)"

ENV DEBIAN_FRONTEND=noninteractive

# ─── Build tools and project dependencies ─────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Compiler and build tools
    g++-14 gcc-14 cmake ninja-build pkg-config make \
    ca-certificates curl xz-utils \
    # CGAL and dependencies
    libgmp-dev libmpfr-dev \
    # Boost
    libboost-all-dev \
    # FFTW3
    libfftw3-dev \
    # Microsoft GSL
    libmsgsl-dev \
    # Google Test
    libgtest-dev \
    # Debug tools
    gdb valgrind \
    # Clang tools (linting, formatting)
    clang-tidy clang-format \
    # Python (for GUI and tests)
    python3 python3-pip python3-venv \
    # Qt6 Designer tools
    qt6-tools-dev-tools \
    # Desktop apps used by the Python GUI
    gedit inkscape \
    # X11 libraries for PyQt6 GUI
    libxcb-xinerama0 libxcb-cursor0 libxkbcommon0 \
    libegl1 libgl1 libx11-xcb1 \
    # Utilities
    git curl sudo locales \
    && rm -rf /var/lib/apt/lists/*

# ─── Locale ───────────────────────────────────────────────────────────────────
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8

# ─── Set GCC 14 as default ───────────────────────────────────────────────────
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

# ─── Non-root user (VS Code convention) ──────────────────────────────────────
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
RUN set -eux; \
    existing_user="$(getent passwd ${USER_UID} | cut -d: -f1 || true)"; \
    existing_group="$(getent group ${USER_GID} | cut -d: -f1 || true)"; \
    if [ -n "${existing_group}" ] && [ "${existing_group}" != "${USERNAME}" ]; then \
    groupmod -n ${USERNAME} "${existing_group}"; \
    elif [ -z "${existing_group}" ]; then \
    groupadd --gid ${USER_GID} ${USERNAME}; \
    fi; \
    if id -u ${USERNAME} >/dev/null 2>&1; then \
    usermod --uid ${USER_UID} --gid ${USER_GID} --home /home/${USERNAME} --move-home --shell /bin/bash ${USERNAME}; \
    elif [ -n "${existing_user}" ]; then \
    usermod --login ${USERNAME} --home /home/${USERNAME} --move-home --shell /bin/bash "${existing_user}"; \
    usermod --gid ${USER_GID} ${USERNAME}; \
    else \
    useradd --uid ${USER_UID} --gid ${USER_GID} -m -s /bin/bash ${USERNAME}; \
    fi; \
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME}

# ─── Python virtual environment ──────────────────────────────────────────────
RUN python3 -m venv /home/${USERNAME}/venv \
    && /home/${USERNAME}/venv/bin/pip install --no-cache-dir \
    protobuf==7.34.1 watchdog pytest PyQt6 \
    && chown -R ${USERNAME}:${USERNAME} /home/${USERNAME}/venv

ENV PATH="/home/${USERNAME}/venv/bin:/usr/lib/qt6/bin:${PATH}"

USER ${USERNAME}
WORKDIR /workspace
