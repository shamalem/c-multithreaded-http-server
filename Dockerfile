# Build stage
FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends gcc make && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY Makefile ./
COPY include/ include/
COPY src/ src/
RUN make

# Runtime stage: slim image with only the built binary + static assets
FROM debian:bookworm-slim
WORKDIR /app
COPY --from=build /src/server ./server
COPY public/ public/

EXPOSE 8080
CMD ["./server"]
