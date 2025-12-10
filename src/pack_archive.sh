#!/bin/bash
set -eu
libs_file="$1"
lib_name="$2"
shift 2

echo "CREATE $lib_name" > libpostgres.mri
# Add our custom object files
while [ $# -gt 0 ]; do
	obj="$1"
       echo "ADDMOD $obj" >> libpostgres.mri;
       shift
done

# Add all PostgreSQL libraries and objects from the generated list
while IFS= read -r file; do
    if [ -n "$file" ]; then
       	case "$file" in
       		../../*)
       			echo "ADDLIB ../vendor/pg18/src/backend/$file" >> libpostgres.mri;
       			;;
       		*.a)
       			echo "ADDLIB ../vendor/pg18/src/backend/$file" >> libpostgres.mri;
       			;;
       		src/*)
       			# Paths like src/timezone/*.o or src/common/*.a
       			echo "ADDMOD ../vendor/pg18/$file" >> libpostgres.mri;
       			;;
       		*.o)
       			# Backend object files like access/brin/brin.o
       			echo "ADDMOD ../vendor/pg18/src/backend/$file" >> libpostgres.mri;
       			;;
       	esac;
    fi;
done < "$libs_file"

echo "SAVE" >> libpostgres.mri
echo "END" >> libpostgres.mri
ar -M < libpostgres.mri
rm -f libpostgres.mri
