# Не просто создаём образ, но даём ему имя build
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# Запуск conan как раньше
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s compiler.libcxx=libstdc++11
#-s build_type=Release

# Папка data больше не нужна
COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 as run

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Скопируем приложение со сборочного контейнера в директорию /app.
# Не забываем также папку data, она пригодится.

COPY --from=build /app/build/game_server /app/
COPY ./data /app/data
COPY ./static /app/static
#ARG GAME_DB_URL=postgres://postgres:Mys3Cr3t@127.0.0.1:5432/postgres
#ENV GAME_DB_URL=$GAME_DB_URL

# Запускаем игровой сервер
#ENTRYPOINT ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static", "--randomize-spawn-points", "--tick-period", "100", "--state-file", "/app/data/save"]
ENTRYPOINT ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static", "--randomize-spawn-points"]