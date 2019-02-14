#!/bin/sh

dd if=../adcs.gz of=adcs_v2.0 bs=1024 count=1 skip=0
dd if=../adcs.gz of=adcs_v2.1 bs=1024 count=1 skip=1
dd if=../adcs.gz of=adcs_v2.2 bs=1024 count=1 skip=2
dd if=../adcs.gz of=adcs_v2.3 bs=1024 count=1 skip=3
dd if=../adcs.gz of=adcs_v2.4 bs=1024 count=1 skip=4

dd if=../adcs-util.gz of=adcs-util_v2.0 bs=1024 count=1 skip=0
dd if=../adcs-util.gz of=adcs-util_v2.1 bs=1024 count=1 skip=1
dd if=../adcs-util.gz of=adcs-util_v2.2 bs=1024 count=1 skip=2
dd if=../adcs-util.gz of=adcs-util_v2.3 bs=1024 count=1 skip=3
dd if=../adcs-util.gz of=adcs-util_v2.4 bs=1024 count=1 skip=4
dd if=../adcs-util.gz of=adcs-util_v2.5 bs=1024 count=1 skip=5
dd if=../adcs-util.gz of=adcs-util_v2.6 bs=1024 count=1 skip=6
