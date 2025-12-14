# Common build configuration shared across all Makefiles
# Include this in each Makefile with: include ../common.mk

CC = musl-gcc
AR = ar
CFLAGS = -static -I"$(PG_INCLUDE)" -O0 -ggdb -fdata-sections -ffunction-sections -Werror=implicit-function-declaration
LDFLAGS = -Wl,--gc-sections

# PostgreSQL paths
PG_ROOT = $(shell cd "$(dir $(lastword $(MAKEFILE_LIST)))/vendor/pg18" && pwd)
PG_INCLUDE = $(PG_ROOT)/src/include
PG_BACKEND_LIBS = $(PG_ROOT)/src/backend/backend-libs.txt
