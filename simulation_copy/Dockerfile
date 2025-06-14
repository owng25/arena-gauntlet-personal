# Use a base image with Ubuntu 20.04 LTS
FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Update the package repository and install necessary dependencies
RUN apt-get update && apt-get install -y \
    ninja-build \
    clang \
    make \
    libpthread-stubs0-dev \
    git \
    cmake \
    build-essential \
    libssl-dev \
    clang-format \
    clang-tidy \
    clang-tools \
    python3 \
    python3-pip

# Install Conan using pip
RUN pip3 install conan==1.66.0

# Add Python scripts to the PATH
ENV PATH="/root/.local/bin:$PATH"

# Create a directory to copy the repository contents
RUN mkdir -p /root/simulation

# Copy the contents of the local repository to the image
COPY . /root/simulation/

# Set the working directory to the simulation directory
WORKDIR /root/simulation

RUN conan profile new default --detect
RUN conan profile update settings.compiler.libcxx=libstdc++11 default

# Build the Simulation program
RUN /root/simulation/LinuxBuild.sh

# Switch to the folder with the executable
WORKDIR /root/simulation/BuildLinux/bin

# If this step fails -> it means you have not copied the json data files
# Put the `data` folder from this repo and rename it to `simulation-json-data`
# github.com/IlluviumGame/json-data/
COPY ./simulation-json-data /root/simulation/simulation-json-data

# Set the json data path for the simulation to work
RUN ./simulation-cli set --json_data_path /root/simulation/simulation-json-data


# Define an environment variable for the JSON content
ENV BATTLE_JSON="{\"battle_json\": \"values\"}"


# Example run command: echo '{battle_json: values}' > battle.json && ./simulation-cli run battle.json
#  Saves the JSON battle file and runs the battle file
CMD echo $BATTLE_JSON > /root/simulation/1.json && cat /root/simulation/1.json  && ./simulation-cli run /root/simulation/1.json


# Set the entry point
#CMD ["/bin/bash"]
