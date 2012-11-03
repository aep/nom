b=s
a=$b
$(a): $(b)
	cc $< -o $@
