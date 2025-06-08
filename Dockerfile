# Build stage using Alpine for smaller size
FROM alpine:3.19 AS builder

# Install build dependencies
RUN apk add --no-cache \
    build-base \
    cmake \
    ninja \
    icu-dev \
    pkgconfig \
    git \
    linux-headers

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Create build directory and configure
RUN mkdir -p build && cd build && \
    cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_CLI=ON \
    -DBUILD_TESTING=OFF

# Build the project
RUN cd build && ninja -j$(nproc)

# Strip the binary to reduce size
RUN strip /app/build/suzume-feedmill

# Runtime stage using Alpine for dynamic linking
FROM alpine:3.19 AS runtime

# Install runtime dependencies
RUN apk add --no-cache icu-libs libstdc++

# Create non-root user  
RUN addgroup -g 1000 suzume && adduser -D -u 1000 -G suzume suzume

# Copy only the binary from builder stage
COPY --from=builder /app/build/suzume-feedmill /usr/local/bin/suzume-feedmill

# Switch to non-root user
USER suzume
WORKDIR /home/suzume

# Set default command
ENTRYPOINT ["suzume-feedmill"]
CMD ["--help"]