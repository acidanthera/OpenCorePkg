/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2018-2019, Download-Fritz. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef BOOT_PICKER_H
#define BOOT_PICKER_H

#include "../OpenCanopy.h"

typedef struct {
  GUI_OBJ_CHILD   Hdr;
  GUI_IMAGE       EntryIcon;
  GUI_IMAGE       Label;
  OC_BOOT_ENTRY   *Context;
  BOOLEAN         CustomIcon;
  UINT8           Index;
  BOOLEAN         ShowLeftShadow;
  INT16           LabelOffset;
} GUI_VOLUME_ENTRY;

typedef struct {
  GUI_OBJ_CHILD Hdr;
  UINT32        SelectedIndex;
} GUI_VOLUME_PICKER;

#endif // BOOT_PICKER_H
