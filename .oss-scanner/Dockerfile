FROM ubuntu:26.04
WORKDIR /src
COPY . .
RUN ./tools/install-deps.sh ubuntu g++ >env.txt

CMD \
    mkdir /build && cd /build && . /src/env.txt && \
    /src/tools/run-test-postgres.sh ubuntu postgres && \
    /src/configure CXXFLAGS='-O0 -std=c++26' && make -j4 check
