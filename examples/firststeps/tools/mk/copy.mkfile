# This tutorial requires an input.txt in the private work directory.
all:V: input.copy

# A pattern rule uses mk variables, not GNU Make automatic-variable syntax.
%.copy: %.txt
	# Expand the actual prerequisite and target selected by mk.
	cp $prereq $target
