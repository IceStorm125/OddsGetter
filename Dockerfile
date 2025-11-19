FROM debian:bookworm

# Установка системных зависимостей, CMake и Conan
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    python3 \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    python3-venv \
    python3-distutils \
    ca-certificates \
    curl \
    git \
    build-essential \
    && update-ca-certificates 

RUN pip3 install --break-system-packages --upgrade pip setuptools wheel && pip3 install --break-system-packages conan

WORKDIR /app

# Копируем только Conan-манифесты сначала — для кэширования зависимостей
COPY conanfile.* ./

# Настройка и установка зависимостей Conan
RUN conan profile detect --force
RUN conan install . --build=missing

# Копируем оставшиеся файлы
COPY CMakeLists.txt ./
COPY src/ ./src/
COPY wait-for-it.sh ./

# Сборка проекта
RUN cd build && cmake .. && make -j4

# Замените 'your_executable' на имя вашего бинарного файла
CMD ["./build/server"]
