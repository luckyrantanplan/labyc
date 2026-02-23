FROM ubuntu:24.04

LABEL maintainer="LabyPath Project"
LABEL description="Development container for LabyPath (C++ / Python)"

ENV DEBIAN_FRONTEND=noninteractive

# ─── Build tools and project dependencies ─────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Compiler and build tools
    g++-14 gcc-14 cmake ninja-build pkg-config make \
    # CGAL and dependencies
    libcgal-dev libgmp-dev libmpfr-dev \
    # Boost
    libboost-all-dev \
    # Protocol Buffers
    libprotobuf-dev protobuf-compiler \
    # FFTW3
    libfftw3-dev \
    # SVG++ and rapidxml_ns
    libsvgpp-dev \
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

# ─── Non-root user (VS Code convention) ──────────────────────────────────────
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000
RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} -s /bin/bash \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME}

# ─── Python virtual environment ──────────────────────────────────────────────
RUN python3 -m venv /home/${USERNAME}/venv \
    && /home/${USERNAME}/venv/bin/pip install --no-cache-dir \
       protobuf watchdog pytest PyQt6 \
    && chown -R ${USERNAME}:${USERNAME} /home/${USERNAME}/venv

ENV PATH="/home/${USERNAME}/venv/bin:${PATH}"

USER ${USERNAME}
WORKDIR /workspace
