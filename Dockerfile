FROM alpine:latest

RUN apk add --no-cache gcc make cmake python3 py3-pip libc-dev

WORKDIR /app

COPY . .

# (Opcional) Comando de depuración para listar el contenido de /app
# RUN ls -la /app

RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make

ENTRYPOINT ["/app/build/sand_bench"]

CMD []