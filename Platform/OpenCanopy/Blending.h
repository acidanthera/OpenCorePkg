/** @file
  This file is part of OpenCanopy, OpenCore GUI.

  Copyright (c) 2021, Marvin Häuser. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef BLENDING_H_
#define BLENDING_H_

#define RGB_APPLY_OPACITY(Rgba, Opacity)  \
  (((Rgba) * (Opacity)) / 0xFF)

#endif // BLENDING_H_
