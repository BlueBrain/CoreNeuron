FROM bbpgitlab.epfl.ch:5050/hpc/personal/bsd-292:builder-latest AS builder
FROM bbpgitlab.epfl.ch:5050/hpc/personal/bsd-292:runtime-latest

# Triggers building the 'builder' image, otherwise it is optimized away
COPY --from=builder /etc/debian_version /etc/debian_version
