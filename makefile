GLSLC := glslangValidator

BUILDDIR := $(abspath build)
RESDIR := $(abspath res)
CTTI := $(BUILDDIR)/ctti
LOADER := $(BUILDDIR)/loader
RENDERER := $(BUILDDIR)/librenderer
GAME := $(BUILDDIR)/libgame
DEBUG := $(BUILDDIR)/libdebug
CLIENT := $(BUILDDIR)/client
SERVER := $(BUILDDIR)/server

SHADER_SRCS = $(RESDIR)/color.vert \
	      $(RESDIR)/color.frag \
	      $(RESDIR)/texture.vert \
	      $(RESDIR)/texture.frag \
	      $(RESDIR)/atlas.vert \
	      $(RESDIR)/atlas.frag
SHADER_SPVS = $(patsubst %, %.spv, $(SHADER_SRCS))

LIB_FLAGS := -shared -fPIC
COMMON_FLAGS := -I src/include -g
include $(wildcard $(BUILDDIR)/*.d)

.DEFAULT_GOAL := all
all: $(BUILDDIR) $(CTTI) $(RENDERER) $(GAME) $(DEBUG) $(LOADER) $(SHADER_SPVS) $(CLIENT) $(SERVER)

$(CTTI): src/ctti/ctti.c src/include/third_party/sds.c
	$(CC) -o $@ $^ $(COMMON_FLAGS)

$(LOADER): src/loader/loader.c src/include/third_party/sds.c src/include/third_party/sds.h
	$(CC) -o $@ $^ $(COMMON_FLAGS) -ldl -lpthread -lglfw -lm -lfreetype -I/usr/include/freetype2

$(RENDERER): src/renderer/renderer.c src/include/third_party/sds.c src/include/third_party/sds.h
	$(CC) -o $@ $^ -lvulkan -lpthread  -lfreetype -I/usr/include/freetype2 $(COMMON_FLAGS) $(LIB_FLAGS)

$(GAME): src/game/game.c src/include/third_party/sds.c src/include/third_party/sds.h
	$(CC) -o $@ $^ -lfreetype -I/usr/include/freetype2 $(COMMON_FLAGS) $(LIB_FLAGS)

$(DEBUG): src/debug/debug.c src/include/third_party/sds.c src/include/third_party/sds.h
	$(CC) -o $@ $^ -lfreetype -I/usr/include/freetype2 $(COMMON_FLAGS) $(LIB_FLAGS)

%.spv: %
	$(GLSLC) -V -o $@ $^

$(CLIENT): net/client.c src/include/third_party/sds.c src/include/third_party/sds.h src/loader/platform/unix.c net/draw.c
	$(CC) -o $@ $^ $(COMMON_FLAGS) -lm -lraylib

$(SERVER): net/server.c src/include/third_party/sds.c src/include/third_party/sds.h src/loader/platform/unix.c net/draw.c
	$(CC) -o $@ $^ $(COMMON_FLAGS) -lm -lraylib

$(BUILDDIR):
	mkdir -p $(BUILDDIR) && ln -sf $(RESDIR) $(BUILDDIR)
