#
# Format: expectedResultFile   expression
#
# Notes:
# 1) comments must start with a "#" in the first colum
# 2) no white space is permitted before the 'expectedResultFile'
# 
#

# Identity
1to10.txt		1to10.txt

# Inversion/Complement
none.txt		I all.txt
all.txt			I none.txt
11to20.txt 		I 1to10.txt
1to10.txt		I ( I 1to10.txt )
11to20.txt		I ( I ( I 1to10.txt ) )

# Union
all.txt			1to10.txt U 11to20.txt
all.txt			even.txt U odd.txt
all.txt			odd.txt U even.txt
none.txt		none.txt U none.txt
even.txt		even.txt U even.txt

# Difference
none.txt		all.txt D all.txt
all.txt			all.txt D none.txt
none.txt		none.txt D none.txt
none.txt		none.txt D all.txt
1to10.txt		all.txt D 11to20.txt
even.txt		all.txt D odd.txt
none.txt		all.txt D 1to10.txt D 11to20.txt

# Intersection
all.txt			all.txt X all.txt
none.txt		all.txt X none.txt
none.txt		none.txt X all.txt
odd.txt			all.txt X odd.txt
1to10.txt		all.txt X 1to10.txt
none.txt		1to10.txt X 11to20.txt

# Grouping
all.txt			even.txt U odd.txt U 1to10.txt
all.txt			( even.txt U odd.txt ) U 1to10.txt
all.txt			even.txt U ( odd.txt U 1to10.txt )

none.txt		even.txt X odd.txt X 1to10.txt
none.txt		( even.txt X odd.txt ) X 1to10.txt
none.txt		even.txt X ( odd.txt X 1to10.txt )

none.txt		all.txt D odd.txt D even.txt
even.txt		all.txt D ( odd.txt D even.txt )
none.txt		( all.txt D odd.txt ) D even.txt

# Exhaustive combinations
even.txt		even.txt U ( 1to10.txt X 11to20.txt )
all.txt			even.txt U ( all.txt X odd.txt )
none.txt		even.txt X ( all.txt X odd.txt )
even.txt		even.txt D ( all.txt X odd.txt )

even.txt		even.txt U ( all.txt D odd.txt )
even.txt		even.txt X ( all.txt D odd.txt )
none.txt		even.txt D ( all.txt D odd.txt )

even.txt		even.txt U ( 1to10.txt D odd.txt )
1to10even.txt		even.txt X ( 1to10.txt D odd.txt )
12to20even.txt		even.txt D ( 1to10.txt D odd.txt )

# Arbitrary compound
even.txt		( 1to10.txt D odd.txt ) U ( 11to20.txt D odd.txt )
odd.txt			( 1to10.txt X odd.txt ) U ( 11to20.txt X odd.txt )
odd.txt			I ( ( 1to10.txt D odd.txt ) U ( 11to20.txt D odd.txt ) )
fourthsMinus12.txt	( all.txt X fourths.txt D thirds.txt )
16and20.txt		( all.txt X fourths.txt D thirds.txt ) X I 1to10.txt
noFourths.txt		I ( all.txt X fourths.txt D thirds.txt )
noFourths.txt		I ( all.txt X fourths.txt D thirds.txt ) U ( even.txt X odd.txt )