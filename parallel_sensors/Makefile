# See user_config.inc for build customizations
-include user_config.inc
include default_config.inc

common_sources = sensirion_config.h sensirion_common.h sensirion_common.c
i2c_sources = sensirion_i2c_hal.h sensirion_i2c.h sensirion_i2c.c
scd4x_sources = scd4x_i2c.h scd4x_i2c.c

i2c_implementation ?= sensirion_i2c_hal.c

CFLAGS = -Os -Wall -fstrict-aliasing -Wstrict-aliasing=1 -Wsign-conversion -fPIC -I.

ifdef CI
    CFLAGS += -Werror
endif

.PHONY: all clean

all: run_parallel

run_parallel: clean
	$(CC) $(CFLAGS) -o $@  ${scd4x_sources} ${i2c_sources} \
		${i2c_implementation} ${common_sources} ${sps30_uart_sources} ${uart_sources} ${sps30_uart_dir}/run_parallel.c

clean:
	$(RM) run_parallel
