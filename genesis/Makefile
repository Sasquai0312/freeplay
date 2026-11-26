# Genesis portable engine — host tests, demos, Thumb size report
ROOT := $(abspath ..)
CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -O2 -Iinclude
SAN ?=
ENGINE_SRCS := engine/event.c engine/random.c engine/scales.c engine/rhythm.c \
               engine/melody.c engine/chords.c engine/arpeggiator.c engine/transform.c \
               engine/transform_gate.c engine/turing.c engine/markov.c engine/mutate.c \
               engine/evolve.c
ENGINE_OBJS := $(ENGINE_SRCS:.c=.o)

.PHONY: all test demos clean thumb-size host-asan zip-demos evolution-demos

all: tests/test_genesis tests/test_evolution tests/test_evo_session tools/gen_demos tools/gen_evolution_demos

engine/%.o: engine/%.c include/genesis.h
	$(CC) $(CFLAGS) $(SAN) -c $< -o $@

tests/test_genesis: tests/test_genesis.c tools/smf_export.c $(ENGINE_OBJS) include/genesis.h include/genesis_smf.h
	$(CC) $(CFLAGS) $(SAN) -Iinclude -o $@ tests/test_genesis.c tools/smf_export.c $(ENGINE_OBJS)

tests/test_evolution: tests/test_evolution.c $(ENGINE_OBJS) include/genesis.h
	$(CC) $(CFLAGS) $(SAN) -Iinclude -o $@ tests/test_evolution.c $(ENGINE_OBJS)

tests/test_evo_session: tests/test_evo_session.c $(ENGINE_OBJS) include/genesis.h include/genesis_evo.h
	$(CC) $(CFLAGS) $(SAN) -Iinclude -o $@ tests/test_evo_session.c $(ENGINE_OBJS)

tools/gen_demos: tools/gen_demos.c tools/smf_export.c $(ENGINE_OBJS)
	$(CC) $(CFLAGS) -Iinclude -o $@ tools/gen_demos.c tools/smf_export.c $(ENGINE_OBJS)

tools/gen_evolution_demos: tools/gen_evolution_demos.c tools/smf_export.c $(ENGINE_OBJS)
	$(CC) $(CFLAGS) -Iinclude -o $@ tools/gen_evolution_demos.c tools/smf_export.c $(ENGINE_OBJS)

test: tests/test_genesis tests/test_evolution tests/test_evo_session
	./tests/test_genesis
	./tests/test_evolution
	./tests/test_evo_session

demos: tools/gen_demos
	./tools/gen_demos examples

evolution-demos: tools/gen_evolution_demos
	./tools/gen_evolution_demos examples/evolution

zip-demos: demos
	cd examples && zip -q -r ../genesis_demos.zip *.mid *.txt && mv ../genesis_demos.zip .
	@echo "wrote examples/genesis_demos.zip"

host-asan:
	$(MAKE) clean
	$(MAKE) SAN='-fsanitize=address,undefined -fno-omit-frame-pointer' test

# ARM Thumb freestanding size (no link against firmware)
THUMB_CC ?= clang
THUMB_CFLAGS = -std=c11 -Os -ffunction-sections -fdata-sections \
	-target thumbv7em-none-eabihf -mcpu=cortex-m7 -mthumb -mfloat-abi=hard \
	-ffreestanding -fno-builtin -Iinclude -DNDEBUG

thumb-size:
	@mkdir -p build/thumb
	@for s in $(ENGINE_SRCS); do \
	  base=$$(basename $$s .c); \
	  $(THUMB_CC) $(THUMB_CFLAGS) -c $$s -o build/thumb/$$base.o; \
	done
	@python3 tools/thumb_size.py build/thumb | tee build/thumb/sizes.txt
	@llvm-size -A build/thumb/*.o >> build/thumb/sizes.txt

clean:
	rm -f $(ENGINE_OBJS) tests/test_genesis tests/test_evolution tests/test_evo_session tools/gen_demos tools/gen_evolution_demos
	rm -rf build/thumb
