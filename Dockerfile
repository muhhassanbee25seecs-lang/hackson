FROM debian:bookworm
ENV DEBIAN_FRONTEND=noninteractive

# Essential packages for Qt5 Multimedia and OpenCV
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libopencv-dev \
    libgtk-3-dev \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev \
    libv4l-dev \
    x11-apps \
    qtbase5-dev \
    qtmultimedia5-dev \
    libqt5multimedia5 \
    libqt5multimedia5-plugins \
    libqt5multimediawidgets5 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .
WORKDIR /app/df

# Step 3: Run qmake and point it to the headers explicitly if needed
# We add the INCLUDEPATH to ensure the compiler finds QCameraImageCapture
RUN qmake . "INCLUDEPATH += /usr/include/qt5/QtMultimedia" && make -j$(nproc)

CMD ["./CPProject2"]