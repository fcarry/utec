FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc make libc6-dev libmicrohttpd-dev libsqlite3-dev libcjson-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY Makefile ./
COPY src ./src
RUN make

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    libmicrohttpd12 libsqlite3-0 libcjson1 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -m -u 1001 app
WORKDIR /app
COPY --from=build /build/server /app/server
RUN mkdir -p /app/data && chown -R app:app /app
USER app
ENV STUDENTS_DB=/app/data/students.db
EXPOSE 8080
CMD ["/app/server", "8080"]
