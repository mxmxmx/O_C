#!/bin/sh
# Run from source directory, e.g. ./resources/oc_version.sh "1.0.0 $(git rev-parse --short HEAD)"

if [ -z "$1" ]; then
	echo "Please specify version string"
	exit 1
fi

cat > OC_version.h <<EOF
#ifndef OC_VERSION_H_
#define OC_VERSION_H_
//
// GENERATED FILE, DO NOT EDIT
//
#define OC_VERSION "$1"
#endif
EOF
