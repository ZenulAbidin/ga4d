APP := ga4d
BIN_DIR := bin
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
SRC := $(sort $(wildcard src/*.c))
OBJ := $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

CC ?= cc
PKG_CONFIG ?= pkg-config
CMAKE ?= cmake
GIT ?= git

WARN := -Wall -Wextra -Wpedantic
CSTD := -std=c99
OPT ?= -O2 -g
CPPFLAGS += -Isrc
CFLAGS += $(CSTD) $(WARN) $(OPT)

GLFW_DIR := vendor/glfw
GLFW_BUILD := vendor/build/glfw
GLFW_LIB := $(abspath $(GLFW_BUILD)/src/libglfw3.a)
GLFW_SRC := $(shell find $(GLFW_DIR)/include $(GLFW_DIR)/src -type f 2>/dev/null)
GLFW_PATCH_STAMP := $(GLFW_BUILD)/.glfw-wayland-null-seat-patched

SYSTEM_GLFW := $(shell $(PKG_CONFIG) --exists glfw3 >/dev/null 2>&1 && echo yes)
VENDORED_WAYLAND := $(shell $(PKG_CONFIG) --exists wayland-client wayland-cursor wayland-egl xkbcommon egl wayland-protocols >/dev/null 2>&1 && command -v wayland-scanner >/dev/null 2>&1 && echo yes)

ifeq ($(SYSTEM_GLFW),yes)
GLFW_CFLAGS := $(shell $(PKG_CONFIG) --cflags glfw3)
GLFW_LIBS := $(shell $(PKG_CONFIG) --libs glfw3)
GLFW_TARGET :=
GLFW_HEADER_TARGET :=
else
GLFW_CFLAGS := -I$(GLFW_DIR)/include
ifeq ($(VENDORED_WAYLAND),yes)
GLFW_WAYLAND_CMAKE := ON
GLFW_WAYLAND_LIBS := $(shell $(PKG_CONFIG) --libs wayland-client wayland-cursor wayland-egl xkbcommon egl)
else
GLFW_WAYLAND_CMAKE := OFF
GLFW_WAYLAND_LIBS :=
endif
GLFW_LIBS := $(GLFW_LIB) -lGL -lm -ldl -lpthread -lX11 -lXrandr -lXi -lXxf86vm -lXcursor -lXinerama
GLFW_LIBS += $(GLFW_WAYLAND_LIBS)
GLFW_TARGET := $(GLFW_LIB)
GLFW_HEADER_TARGET := $(GLFW_DIR)/CMakeLists.txt
endif

CPPFLAGS += $(GLFW_CFLAGS)
LDLIBS += $(GLFW_LIBS) -lGL -lm

.PHONY: all check run smoke wayland-smoke headless-smoke vendor-glfw clean distclean print-deps

all: $(BIN_DIR)/$(APP)

$(BIN_DIR)/$(APP): $(OBJ) $(GLFW_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

$(OBJ_DIR)/%.o: src/%.c $(GLFW_HEADER_TARGET)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(GLFW_DIR)/CMakeLists.txt:
	@mkdir -p vendor
	$(GIT) clone --depth 1 --branch 3.4 https://github.com/glfw/glfw.git $(GLFW_DIR)

$(GLFW_PATCH_STAMP): $(GLFW_DIR)/CMakeLists.txt patches/glfw-3.4-wayland-null-seat.patch
	@mkdir -p $(GLFW_BUILD)
	@if grep -q '_glfw.wl.seat &&' $(GLFW_DIR)/src/wl_init.c; then \
		echo "GLFW Wayland null-seat patch already applied"; \
	else \
		patch -d $(GLFW_DIR) -p1 < patches/glfw-3.4-wayland-null-seat.patch; \
	fi
	@touch $@

$(GLFW_LIB): $(GLFW_PATCH_STAMP) $(GLFW_DIR)/CMakeLists.txt Makefile $(GLFW_SRC)
	$(CMAKE) -S $(GLFW_DIR) -B $(GLFW_BUILD) \
		-DGLFW_BUILD_DOCS=OFF \
		-DGLFW_BUILD_EXAMPLES=OFF \
		-DGLFW_BUILD_TESTS=OFF \
		-DGLFW_INSTALL=OFF \
		-DGLFW_BUILD_WAYLAND=$(GLFW_WAYLAND_CMAKE) \
		-DGLFW_BUILD_X11=ON \
		-DCMAKE_BUILD_TYPE=Release
	$(CMAKE) --build $(GLFW_BUILD) --target glfw --parallel

vendor-glfw: $(GLFW_LIB)

check: $(BIN_DIR)/$(APP)
	$(BIN_DIR)/$(APP) --self-test
	$(BIN_DIR)/$(APP) --validate-visuals

run: $(BIN_DIR)/$(APP)
	$(BIN_DIR)/$(APP)

smoke: $(BIN_DIR)/$(APP)
	$(BIN_DIR)/$(APP) --smoke-frames 3

wayland-smoke: $(BIN_DIR)/$(APP)
	env -u DISPLAY GA4D_GLFW_PLATFORM=wayland XDG_RUNTIME_DIR=/tmp/ga4d-runtime WAYLAND_DISPLAY=ga4d-wayland $(BIN_DIR)/$(APP) --smoke-frames 3

headless-smoke: $(BIN_DIR)/$(APP)
	$(BIN_DIR)/$(APP) --headless-smoke 3

print-deps:
	@echo "SYSTEM_GLFW=$(SYSTEM_GLFW)"
	@echo "VENDORED_WAYLAND=$(VENDORED_WAYLAND)"
	@echo "GLFW_CFLAGS=$(GLFW_CFLAGS)"
	@echo "GLFW_LIBS=$(GLFW_LIBS)"

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

distclean: clean
	rm -rf vendor/build

-include $(DEP)
