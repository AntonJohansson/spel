GLSLC := glslc

BUILDDIR := $(abspath build)
RESDIR := $(abspath res)
CTTI := $(BUILDDIR)/ctti
LOADER := $(BUILDDIR)/loader
RENDERER := $(BUILDDIR)/librenderer
GAME := $(BUILDDIR)/libgame

SHADER_SRCS = $(RESDIR)/vert.vert \
	      $(RESDIR)/frag.frag
SHADER_SPVS = $(patsubst %, %.spv, $(SHADER_SRCS))

LIB_FLAGS := -shared -fPIE -fPIC
COMMON_FLAGS := -I src/include -g -MMD
include $(wildcard $(BUILDDIR)/*.d)

.DEFAULT_GOAL := all
all: $(BUILDDIR) $(CTTI) $(RENDERER) $(GAME) $(LOADER) $(SHADER_SPVS)

$(CTTI): src/ctti/ctti.c
	$(CC) -o $@ src/ctti/ctti.c $(COMMON_FLAGS)

$(LOADER): src/loader/loader.c
	$(CC) -o $@ src/loader/loader.c $(COMMON_FLAGS) -ldl -lpthread -lglfw

$(RENDERER): src/renderer/renderer.c
	$(CC) -o $@ src/renderer/renderer.c -lvulkan -lpthread $(COMMON_FLAGS) $(LIB_FLAGS)

$(GAME): src/game/game.c
	$(CC) -o $@ src/game/game.c $(COMMON_FLAGS) $(LIB_FLAGS)

%.spv: %
	$(GLSLC) -o $@ $^

$(BUILDDIR):
	mkdir -p $(BUILDDIR) && ln -s $(RESDIR) $(BUILDDIR)/res
