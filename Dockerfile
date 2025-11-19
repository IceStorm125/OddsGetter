# ---------------------------------------------------------
# 1) Build stage
# ---------------------------------------------------------
FROM debian:bookworm AS builder

# Установка build-зависимостей
RUN apt-get update && apt-get install -y \
    g++ cmake make \
    python3 python3-pip python3-venv python3-setuptools python3-wheel \
    git curl \
    build-essential \
    libssl-dev zlib1g-dev libcurl4-openssl-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Установка Conan
RUN pip3 install --break-system-packages --upgrade pip setuptools wheel
RUN pip3 install --break-system-packages conan

WORKDIR /app

# Копируем только Conan-манифесты для кэширования
COPY conanfile.* ./

RUN conan profile detect --force
RUN conan install . --build=missing

# Копируем оставшиеся файлы
COPY CMakeLists.txt ./
COPY src/ ./src/
COPY wait-for-it.sh ./

# Сборка проекта
RUN mkdir -p build && cd build && cmake .. && make -j4

# ---------------------------------------------------------
# 2) Runtime stage (минимальный образ)
# ---------------------------------------------------------
FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y \
    libssl3 zlib1g libcurl4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Копируем собранный бинарник и скрипт wait-for-it.sh
COPY --from=builder /app/build/OddsGetter .
COPY --from=builder /app/wait-for-it.sh .

CMD ["./OddsGetter"]
