# Build a ready-to-ship headless MultiShip server for Linux, from any host with
# Docker (including Windows + Docker Desktop using Linux containers).
#
# The Ubuntu 22.04 base gives an older glibc, so the resulting binary also runs
# on newer distros (glibc is backward- but not forward-compatible).
#
# Build the runnable image:
#   docker build -t multiship-server .
#
# Run it (map whatever host port you want to the server port):
#   docker run --rm -p 43384:43384 multiship-server                  # default
#   docker run --rm -p 50000:50000 multiship-server --port 50000
#   docker stop <container>   # SIGTERM -> graceful shutdown
#
# Or extract just the Linux binary instead of shipping the image:
#   docker build --target export --output type=local,dest=./out .
#   -> ./out/MultiShipCLI

# ---- build stage ------------------------------------------------------------
FROM ubuntu:22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build pkg-config \
        libsdl2-dev libsdl2-net-dev nlohmann-json3-dev libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# GUI off -> no ImGui fetch, only the CLI is built.
RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DMULTISHIP_BUILD_GUI=OFF \
 && cmake --build build --target MultiShipCLI -j

# ---- export stage (binary only) --------------------------------------------
# `docker build --target export --output type=local,dest=./out .` writes the
# bare binary to ./out/MultiShipCLI without producing an image.
FROM scratch AS export
COPY --from=build /src/build/MultiShipCLI /MultiShipCLI

# ---- runtime stage (default) ------------------------------------------------
FROM ubuntu:22.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive
# Runtime shared libs the binary links against. libspdlog1 pulls in libfmt8;
# SDL2/SDL2_net are needed for the networking. (nlohmann-json is header-only.)
RUN apt-get update && apt-get install -y --no-install-recommends \
        libsdl2-2.0-0 libsdl2-net-2.0-0 libspdlog1 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/MultiShipCLI /usr/local/bin/MultiShipCLI

# Documentary: the actual listening port is whatever you pass below / map with -p.
EXPOSE 43384

# Exec form -> the server is PID 1 and receives SIGINT/SIGTERM directly, so
# `docker stop` triggers the graceful shutdown handler in main_cli.cpp.
ENTRYPOINT ["/usr/local/bin/MultiShipCLI"]
CMD ["--port", "43384"]
