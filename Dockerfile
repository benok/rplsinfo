# syntax=docker/dockerfile:1
FROM debian:12-slim AS builder
RUN DEBIAN_FRONTEND=noninteractive apt update && apt install -y make g++ libc6-dev
WORKDIR /build
COPY . /build
RUN make config=release

FROM gcr.io/distroless/base-debian12:latest
WORKDIR /app
COPY --from=builder /build/bin/Release/rplsinfo /usr/local/bin/
ENTRYPOINT [ "rplsinfo" ]
