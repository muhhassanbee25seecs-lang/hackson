FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive

# Added qt6-multimedia-dev and libqt6multimediawidgets6
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libopencv-dev \
    libgtk-3-dev \
    pkg-config \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev \
    libv4l-dev \
    x11-apps \
    qt6-base-dev \
    qt6-base-dev-tools \
    qt6-multimedia-dev \
    libqt6multimediawidgets6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy your local code
COPY . .

# Move into the subdirectory with the .pro file
WORKDIR /app/df

# Build using qmake6
RUN qmake6 . && make -j$(nproc)

# Execute the binary
CMD ["./CPProject2"]