cd vendor/pg18
git diff --name-only | while read -r line; do
	dir=$(dirname $line);
	fname=$(basename $line);
	mkdir -p ../../patches/$dir;
	git diff $line > ../../patches/$dir/${fname}.patch;
done
