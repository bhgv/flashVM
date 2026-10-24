# Direct native fallback; CMake 3.10 remains the complete workspace build path.
.PHONY: all mk iyacc cocor compilers as1 as2 as3 limbo linker
all: compilers linker iyacc
mk iyacc cocor compilers as1 as2 as3 limbo linker:
	bash tools/build-native.sh $@
