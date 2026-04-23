FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive

# Comprehensive list of dependencies for Qt6 Multimedia + OpenCV
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
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
    libqt6multimedia6 \
    libqt6multimediawidgets6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the whole project
COPY . .

# Move to the directory containing your .pro file
WORKDIR /app/df

# Build
RUN qmake6 . && make -j$(nproc)

# Run the system
CMD ["./CPProject2"]