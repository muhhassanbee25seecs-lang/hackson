FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential cmake git libopencv-dev libgtk-3-dev \
    pkg-config libavcodec-dev libavformat-dev libswscale-dev \
    libv4l-dev x11-apps && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Copy EVERYTHING shown in your VS Code sidebar
COPY . .

# 2. Move INTO the directory where the code and CMakeLists.txt actually are
WORKDIR /app/df

# 3. Build from THERE
RUN qmake *.pro && make
CMD ["./CPProject2"]