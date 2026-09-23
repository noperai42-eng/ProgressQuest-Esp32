CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Iapp/include -g
APP_SRC := app/src/rng.c app/src/names.c app/src/sim.c app/src/anim.c app/src/roster.c
BUILD := build
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS := $(shell sdl2-config --libs 2>/dev/null)

IDF_IMAGE ?= espressif/idf:v5.5.2

PORT ?= /dev/cu.usbmodem1101

.PHONY: all sim test gfx device device-cli paths harness harness-device clean

all: test sim gfx

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/tami-sim: $(APP_SRC) host/main.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(APP_SRC) host/main.c -lm

$(BUILD)/test_sim: $(APP_SRC) tests/test_sim.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(APP_SRC) tests/test_sim.c -lm

sim: $(BUILD)/tami-sim

gfx: $(BUILD)/tami-gfx

$(BUILD)/tami-gfx: $(APP_SRC) host/gfx.c vendor/stb_easy_font.h | $(BUILD)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $(APP_SRC) host/gfx.c $(SDL_LIBS) -lm

$(BUILD)/milestones: $(APP_SRC) tests/milestones.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(APP_SRC) tests/milestones.c -lm

$(BUILD)/tami-paths: $(APP_SRC) tests/paths.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(APP_SRC) tests/paths.c -lm

paths: $(BUILD)/tami-paths
	$(BUILD)/tami-paths

harness: paths
	./tools/venv/bin/python scripts/progress-harness.py --desk

harness-device:
	./tools/venv/bin/python scripts/progress-harness.py --device

test: $(BUILD)/test_sim $(BUILD)/milestones
	$(BUILD)/test_sim
	$(BUILD)/milestones

device:
	python3 scripts/pack-device-art.py
	docker run --rm -v "$(CURDIR):/project" -w /project/device $(IDF_IMAGE) \
		idf.py build merge-bin

device-cli:
	./tools/venv/bin/python scripts/device-cli.py $(PORT)

clean:
	rm -rf $(BUILD) device/build device/sdkconfig device/managed_components
