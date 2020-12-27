#  Copyright (c) 2020, Mike Beaton. All rights reserved.
#  SPDX-License-Identifier: BSD-3-Clause

targets := mapped.plist test.c test.h
debug_targets := _test.c _test.h _OcConfigurationLib.c _OcConfigurationLib.h

.PHONY: debug
debug: $(debug_targets)

.PHONY: clean
clean:
	rm -f $(targets) $(debug_targets)

.PHONY: live
live:
	./PlistToConfig.py -f 0x21 Template.plist -c OcConfigurationLib.c -h ../../Include/Acidanthera/Library/OcConfigurationLib.h > ../../Docs/SampleCustom.plist
	./CheckSchema.py OcConfigurationLib.c

$(targets) &: PlistToConfig.py Template.plist
	./PlistToConfig.py -f 0xf Template.plist -c test.c -h test.h > mapped.plist

_%.c: %.c
	./StripSpace.py $^ > $@

vpath %.h ../../Include/Acidanthera/Library

_%.h: %.h
	./StripSpace.py $^ > $@
