FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive

# Added qt6-base-dev and qt6-base-dev-tools
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
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy everything from your local folder to the container
COPY . .

# Move into the subdirectory where your .pro file lives
WORKDIR /app/df

# Build using qmake6
RUN qmake6 . && make -j$(nproc)

# Execute the binary
CMD ["./CPProject2"]