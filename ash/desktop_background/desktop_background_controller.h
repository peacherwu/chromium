// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DESKTOP_BACKGROUND_DESKTOP_BACKGROUND_CONTROLLER_H_
#define ASH_DESKTOP_BACKGROUND_DESKTOP_BACKGROUND_CONTROLLER_H_
#pragma once

#include "ash/ash_export.h"
#include "base/basictypes.h"

class SkBitmap;

namespace ash {

// A class to listen for login and desktop background change events and set the
// corresponding default wallpaper in Aura shell.
class ASH_EXPORT DesktopBackgroundController {
 public:
  enum BackgroundMode {
    BACKGROUND_IMAGE,
    BACKGROUND_SOLID_COLOR
  };

  DesktopBackgroundController();
  virtual ~DesktopBackgroundController();

  // Get the desktop background mode.
  BackgroundMode desktop_background_mode() const {
    return desktop_background_mode_;
  }

  // Change the desktop background image to wallpaper with |index|.
  void OnDesktopBackgroundChanged(int index);

  // Sets the desktop background to image mode and create a new background
  // widget with |wallpaper|.
  void SetDesktopBackgroundImageMode(const SkBitmap& wallpaper);

  // Sets the desktop background to image mode and create a new background
  // widget with default wallpaper.
  void SetDefaultDesktopBackgroundImage();

  // Sets the desktop background to image mode and create a new background
  // widget with previous selected wallpaper at run time.
  void SetPreviousDesktopBackgroundImage();

  // Sets the desktop background to solid color mode and create a solid color
  // layout.
  void SetDesktopBackgroundSolidColorMode();

 private:
  // We need to cache the previously used wallpaper index. So when users switch
  // desktop background color mode at run time, we can directly switch back to
  // the user selected wallpaper in image mode.
  int previous_wallpaper_index_;

  // Can change at runtime.
  BackgroundMode desktop_background_mode_;

  DISALLOW_COPY_AND_ASSIGN(DesktopBackgroundController);
};

}  // namespace ash

#endif  // ASH_DESKTOP_BACKGROUND_DESKTOP_BACKGROUND_CONTROLLER_H_
