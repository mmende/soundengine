# This file is generated by gyp; do not edit.

TOOLSET := target
TARGET := soundengine
DEFS_Debug := \
	'-DNODE_GYP_MODULE_NAME=soundengine' \
	'-D_DARWIN_USE_64_BIT_INODE=1' \
	'-D_LARGEFILE_SOURCE' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DBUILDING_NODE_EXTENSION' \
	'-DDEBUG' \
	'-D_DEBUG'

# Flags passed to all source files.
CFLAGS_Debug := \
	-O0 \
	-gdwarf-2 \
	-mmacosx-version-min=10.7 \
	-arch x86_64 \
	-Wall \
	-Wendif-labels \
	-W \
	-Wno-unused-parameter

# Flags passed to only C files.
CFLAGS_C_Debug := \
	-fno-strict-aliasing

# Flags passed to only C++ files.
CFLAGS_CC_Debug := \
	-std=gnu++0x \
	-fno-rtti \
	-fno-exceptions \
	-fno-threadsafe-statics \
	-std=c++11 \
	-stdlib=libc++

# Flags passed to only ObjC files.
CFLAGS_OBJC_Debug :=

# Flags passed to only ObjC++ files.
CFLAGS_OBJCC_Debug :=

INCS_Debug := \
	-I/Users/martinmende/.node-gyp/5.9.1/include/node \
	-I/Users/martinmende/.node-gyp/5.9.1/src \
	-I/Users/martinmende/.node-gyp/5.9.1/deps/uv/include \
	-I/Users/martinmende/.node-gyp/5.9.1/deps/v8/include \
	-I$(srcdir)/node_modules/nan \
	-I/Users/martinmende/Documents/development/soundengine/src \
	-I$(srcdir)/-I/usr/local/Cellar/portaudio/19.6.0/include \
	-I$(srcdir)/-I/usr/local/Cellar/fftw/3.3.4_1/include

DEFS_Release := \
	'-DNODE_GYP_MODULE_NAME=soundengine' \
	'-D_DARWIN_USE_64_BIT_INODE=1' \
	'-D_LARGEFILE_SOURCE' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DBUILDING_NODE_EXTENSION'

# Flags passed to all source files.
CFLAGS_Release := \
	-Os \
	-gdwarf-2 \
	-mmacosx-version-min=10.7 \
	-arch x86_64 \
	-Wall \
	-Wendif-labels \
	-W \
	-Wno-unused-parameter

# Flags passed to only C files.
CFLAGS_C_Release := \
	-fno-strict-aliasing

# Flags passed to only C++ files.
CFLAGS_CC_Release := \
	-std=gnu++0x \
	-fno-rtti \
	-fno-exceptions \
	-fno-threadsafe-statics \
	-std=c++11 \
	-stdlib=libc++

# Flags passed to only ObjC files.
CFLAGS_OBJC_Release :=

# Flags passed to only ObjC++ files.
CFLAGS_OBJCC_Release :=

INCS_Release := \
	-I/Users/martinmende/.node-gyp/5.9.1/include/node \
	-I/Users/martinmende/.node-gyp/5.9.1/src \
	-I/Users/martinmende/.node-gyp/5.9.1/deps/uv/include \
	-I/Users/martinmende/.node-gyp/5.9.1/deps/v8/include \
	-I$(srcdir)/node_modules/nan \
	-I/Users/martinmende/Documents/development/soundengine/src \
	-I$(srcdir)/-I/usr/local/Cellar/portaudio/19.6.0/include \
	-I$(srcdir)/-I/usr/local/Cellar/fftw/3.3.4_1/include

OBJS := \
	$(obj).target/$(TARGET)/src/SoundEngine.o \
	$(obj).target/$(TARGET)/src/WindowFunction.o

# Add to the list of files we specially track dependencies for.
all_deps += $(OBJS)

# CFLAGS et al overrides must be target-local.
# See "Target-specific Variable Values" in the GNU Make manual.
$(OBJS): TOOLSET := $(TOOLSET)
$(OBJS): GYP_CFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_C_$(BUILDTYPE))
$(OBJS): GYP_CXXFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_CC_$(BUILDTYPE))
$(OBJS): GYP_OBJCFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_C_$(BUILDTYPE)) $(CFLAGS_OBJC_$(BUILDTYPE))
$(OBJS): GYP_OBJCXXFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_CC_$(BUILDTYPE)) $(CFLAGS_OBJCC_$(BUILDTYPE))

# Suffix rules, putting all outputs into $(obj).

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(srcdir)/%.cpp FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

# Try building from generated source, too.

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj).$(TOOLSET)/%.cpp FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj)/%.cpp FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

# End of this set of suffix rules
### Rules for final target.
LDFLAGS_Debug := \
	-stdlib=libc++ \
	-undefined dynamic_lookup \
	-Wl,-search_paths_first \
	-mmacosx-version-min=10.7 \
	-arch x86_64 \
	-L$(builddir)

LIBTOOLFLAGS_Debug := \
	-stdlib=libc++ \
	-undefined dynamic_lookup \
	-Wl,-search_paths_first

LDFLAGS_Release := \
	-stdlib=libc++ \
	-undefined dynamic_lookup \
	-Wl,-search_paths_first \
	-mmacosx-version-min=10.7 \
	-arch x86_64 \
	-L$(builddir)

LIBTOOLFLAGS_Release := \
	-stdlib=libc++ \
	-undefined dynamic_lookup \
	-Wl,-search_paths_first

LIBS := \
	-lportaudio \
	-L/usr/local/Cellar/fftw/3.3.4_1/lib \
	-lfftw3 \
	-framework AudioToolbox \
	-framework AudioUnit \
	-framework Carbon

$(builddir)/soundengine.node: GYP_LDFLAGS := $(LDFLAGS_$(BUILDTYPE))
$(builddir)/soundengine.node: LIBS := $(LIBS)
$(builddir)/soundengine.node: GYP_LIBTOOLFLAGS := $(LIBTOOLFLAGS_$(BUILDTYPE))
$(builddir)/soundengine.node: TOOLSET := $(TOOLSET)
$(builddir)/soundengine.node: $(OBJS) FORCE_DO_CMD
	$(call do_cmd,solink_module)

all_deps += $(builddir)/soundengine.node
# Add target alias
.PHONY: soundengine
soundengine: $(builddir)/soundengine.node

# Short alias for building this executable.
.PHONY: soundengine.node
soundengine.node: $(builddir)/soundengine.node

# Add executable to "all" target.
.PHONY: all
all: $(builddir)/soundengine.node

