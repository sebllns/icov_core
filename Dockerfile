# GCC support can be specified at major, minor, or micro version
# (e.g. 8, 8.2 or 8.2.0).
# See https://hub.docker.com/r/library/gcc/ for all supported GCC
# tags from Docker Hub.
# See https://docs.docker.com/samples/library/gcc/ for more on how to use this image
FROM ubuntu:latest  AS build
# FROM alpine:3.19 AS build
# This command compiles your app using GCC, adjust for your source code
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        git \
        clang \
        autoconf \
        automake \
        make \
        #linux-headers \
        gettext \
        libtool \
        libusb-dev \
        #libusb-compat-dev \
        pkgconf \
        openssl \
        cmake \
       # boost1.82 \
        libboost-all-dev \
        dpkg \
        ccache \
        python3

RUN ln -sf /usr/bin/clang /usr/bin/cc \
  && ln -sf /usr/bin/clang++ /usr/bin/c++ \
  && update-alternatives --install /usr/bin/cc cc /usr/bin/clang 10\
  && update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 10\
  && update-alternatives --auto cc \
  && update-alternatives --auto c++ \
  && update-alternatives --display cc \
  && update-alternatives --display c++ \
  && ls -l /usr/bin/cc /usr/bin/c++ \
  && cc --version \
  && c++ --version

WORKDIR /icov

RUN git clone --recurse-submodules -j8 https://gitlab.inria.fr/icov/src/core.git


# These commands copy your files into the specified directory in the image
# and set that as the working location
# COPY lib/ ./lib/
# COPY app/ ./app/
# COPY third-party/ ./third-party/
# COPY data/ ./data/
# COPY .cmake/ ./.cmake/
# COPY CMakeLists.txt .



WORKDIR /icov/core/build

RUN cmake -DCMAKE_BUILD_TYPE=Release .. | tee icov_config.log
RUN cmake --build . --parallel 8 | tee icov_build.log

# This command runs your application, comment out this line to compile only
# CMD ["./icov"]

LABEL Name=icov Version=1.8.0



# WORKDIR /simplehttpserver

# COPY src/ ./src/
# COPY CMakeLists.txt .

# WORKDIR /simplehttpserver/build

# RUN cmake -DCMAKE_BUILD_TYPE=Release .. && \
#     cmake --build . --parallel 8

# ########################################################################################################################
# # simplehttpserver image
# ########################################################################################################################

# FROM alpine:3.17.0

# RUN apk update && \
#     apk add --no-cache \
#     libstdc++ \
#     boost1.80-program_options=1.80.0-r3

# RUN addgroup -S shs && adduser -S shs -G shs
# USER shs

# COPY --chown=shs:shs --from=build \
#     ./simplehttpserver/build/src/simplehttpserver \
#     ./app/

# ENTRYPOINT [ "./app/simplehttpserver" ]