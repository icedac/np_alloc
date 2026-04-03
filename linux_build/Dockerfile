FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy entire repo
COPY . /repo

# Work in linux_build directory
WORKDIR /repo/linux_build

# Build
RUN make clean && make 2>&1

# Run benchmark
CMD ["./np_alloc_bench"]
