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
    protobuf watchdog pytest PyQt6 \
    && chown -R ${USERNAME}:${USERNAME} /home/${USERNAME}/venv

ENV PATH="/home/${USERNAME}/venv/bin:/usr/lib/qt6/bin:${PATH}"

USER ${USERNAME}
WORKDIR /workspace
