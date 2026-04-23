FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential cmake git libopencv-dev libgtk-3-dev \
    pkg-config libavcodec-dev libavformat-dev libswscale-dev \
    libv4l-dev x11-apps && rm -rf /var/lib/apt/lists/*
WORKDIR /app

# Copy EVERYTHING shown in your VS Code sidebar
COPY . .

# 2. Move into your project subdirectory
WORKDIR /app/df

# 3. Build using qmake
# We use -makefile to ensure it generates a clean Makefile
RUN qmake6 . && make -j$(nproc)

# 4. Set the path to your executable
# Ensure the name matches the TARGET defined in your .pro file
CMD ["./CPProject2"]