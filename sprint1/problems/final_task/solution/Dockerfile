# �� ������ ������ �����, �� ��� ��� ��� build
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# ������ conan ��� ������
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing

# ����� data ������ �� �����
COPY ./src /app/src
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

# ������ ��������� � ��� �� ����������
FROM ubuntu:22.04 as run

# �������� ������������ www
RUN groupadd -r www && useradd -r -g www www
USER www

# ��������� ���������� �� ���������� ���������� � ���������� /app.
# �� �������� ����� ����� data, ��� ����������.
COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data

# ��������� ������� ������
ENTRYPOINT ["/app/game_server", "/app/data/config.json"]