#!/bin/bash
docker run --mount type=bind,src=$(pwd),target=/var/code/spacefn_xkb -i -t spacefn_build_env:latest bash

