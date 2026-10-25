# Default virtual target asks mk to produce a real output file.
all:V: hello.txt

# No prerequisite: create this tutorial output in the current working directory.
hello.txt:
	# The recipe is interpreted by the bundled mk host shell adapter.
	echo 'Hello from the source-built Inferno mk' > hello.txt
