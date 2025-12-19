set -euo pipefail

for repo in pg18 proj; do
	[ -d "$repo" ] && echo "you need to rm -rf $repo first to ensure no old patches are left" && exit 1
done

for repo in pg18 proj; do
	cd ../vendor/$repo
	git diff --name-only | while read -r line; do
		dir=$(dirname $line);
		fname=$(basename $line);
		mkdir -p ../../patches/$repo/$dir;
		git diff $line > ../../patches/$repo/$dir/${fname}.patch;
	done
	cd -
done
