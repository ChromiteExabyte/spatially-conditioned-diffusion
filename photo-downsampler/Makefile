CC   ?= cc
EMCC  = /opt/emsdk/upstream/emscripten/emcc

# GCC 13 uses -std=c2x (pre-ratification name for C23)
# Clang / emcc accept -std=c23
CSTD_GCC  = -std=c2x
CSTD_EMCC = -std=c23

CFLAGS_COMMON = -O2 -Wall -Wextra -Isrc -Ivendor
CFLAGS        = $(CSTD_GCC)  $(CFLAGS_COMMON)
EMFLAGS       = $(CSTD_EMCC) $(CFLAGS_COMMON)

LDFLAGS = -lm

NATIVE_SRCS = src/main.c src/image.c src/resample.c src/effects.c
WASM_SRCS   = src/wasm_api.c src/image.c src/resample.c src/effects.c

TARGET = photodown

# Emscripten link flags
EMLINK = \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=PhotoDown \
  -s EXPORTED_FUNCTIONS='["_malloc","_free","_process_image","_get_output_ptr","_get_output_len","_free_output"]' \
  -s EXPORTED_RUNTIME_METHODS='["HEAPU8"]' \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s ENVIRONMENT=web \
  --no-entry

.PHONY: all wasm serve clean

all: $(TARGET)

$(TARGET): $(NATIVE_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

wasm: web/photodown.js

web/photodown.js: $(WASM_SRCS)
	@mkdir -p web
	$(EMCC) $(EMFLAGS) $(EMLINK) -o $@ $^ $(LDFLAGS)
	@echo "WASM build complete → web/"

serve: web/photodown.js
	@echo "Serving at http://localhost:8080"
	python3 -m http.server 8080 --directory web

clean:
	rm -f $(TARGET) web/photodown.js web/photodown.wasm

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
