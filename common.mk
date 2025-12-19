# Common build configuration shared across all Makefiles
# Include this in each Makefile with: include ../common.mk

CC = musl-gcc
CXX = musl-gcc
AR = ar
OPTLEVEL = -O0 -ggdb

CC  = $$HOME/Downloads/x86_64-linux-musl-native/bin/x86_64-linux-musl-gcc
CXX = $$HOME/Downloads/x86_64-linux-musl-native/bin/x86_64-linux-musl-g++
AR = ar
OPTLEVEL = -O2

CFLAGS =   -static $(OPTLEVEL) -fdata-sections -ffunction-sections -Werror=implicit-function-declaration
CXXFLAGS = -static $(OPTLEVEL) -fdata-sections -ffunction-sections
LDFLAGS = -Wl,--gc-sections

# PostgreSQL paths
PG_ROOT = $(shell cd "$(dir $(lastword $(MAKEFILE_LIST)))/vendor/pg18" && pwd)
PG_INCLUDE = $(PG_ROOT)/src/include
PG_BACKEND_LIBS = $(PG_ROOT)/src/backend/backend-libs.txt
