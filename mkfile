# Native Inferno mk rules. Run tools/mk/bootstrap.sh once, or use build-workspace.sh.
# These recipes compile directly through CC/ar; there is no CMake delegation.
# BUILD_ROOT, CC, AR and standard compiler flags are inherited by the shell builder.
all:V: compilers linker iyacc

mk:V:
	bash tools/build-native.sh mk

iyacc:V:
	bash tools/build-native.sh iyacc

cocor:V:
	bash tools/build-native.sh cocor

compilers:V:
	bash tools/build-native.sh compilers

as1:V:
	bash tools/build-native.sh as1

as2:V:
	bash tools/build-native.sh as2

as3:V:
	bash tools/build-native.sh as3

limbo:V:
	bash tools/build-native.sh limbo

linker:V:
	bash tools/build-native.sh linker
