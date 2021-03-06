// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_button_cell.h"
#import "chrome/browser/ui/cocoa/cocoa_test_helper.h"
#include "ui/gfx/mac/nsimage_cache.h"

namespace {

class BookmarkBarFolderButtonCellTest : public CocoaTest {
};

// Basic creation.
TEST_F(BookmarkBarFolderButtonCellTest, Create) {
  scoped_nsobject<BookmarkBarFolderButtonCell> cell;
  cell.reset([[BookmarkBarFolderButtonCell buttonCellForNode:nil
                                                 contextMenu:nil
                                                    cellText:nil
                                                   cellImage:nil] retain]);
  EXPECT_TRUE(cell);
}

TEST_F(BookmarkBarFolderButtonCellTest, FaviconPositioning) {
  NSRect frame = NSMakeRect(0, 0, 50, 30);
  scoped_nsobject<NSButton> view([[NSButton alloc] initWithFrame:frame]);
  scoped_nsobject<NSButton> folder_view(
      [[NSButton alloc] initWithFrame:frame]);

  ASSERT_TRUE(view.get());
  ASSERT_TRUE(folder_view.get());

  scoped_nsobject<NSImage> image(
      [gfx::GetCachedImageWithName(@"nav.pdf") retain]);
  ASSERT_TRUE(image.get());

  scoped_nsobject<BookmarkButtonCell> cell(
      [[BookmarkButtonCell alloc] initTextCell:@"Testing"]);
  scoped_nsobject<BookmarkBarFolderButtonCell> folder_cell(
      [[BookmarkBarFolderButtonCell buttonCellForNode:nil
                                          contextMenu:nil
                                             cellText:@"Testing"
                                            cellImage:image] retain]);

  ASSERT_TRUE(cell.get());
  ASSERT_TRUE(folder_cell.get());

  [view setCell:cell.get()];
  [folder_view setCell:folder_cell.get()];

  [[test_window() contentView] addSubview:view];
  [[test_window() contentView] addSubview:folder_view];

  NSRect rect = NSMakeRect(20, 20, 20, 20);

  [cell setBookmarkCellText:@"" image:image];
  float cell_x_without_title = ([cell imageRectForBounds:rect]).origin.x;
  float cell_width_without_title = ([cell cellSize]).width;

  [cell setBookmarkCellText:@"test" image:image];
  float cell_x_with_title = ([cell imageRectForBounds:rect]).origin.x;
  float cell_width_with_title = ([cell cellSize]).width;

  EXPECT_LT(cell_x_without_title, cell_x_with_title);
  EXPECT_LT(cell_width_without_title, cell_width_with_title);

  [folder_cell setBookmarkCellText:@"" image:image];
  float folder_cell_x_without_title = ([cell imageRectForBounds:rect]).origin.x;
  float folder_cell_width_without_title = ([cell cellSize]).width;

  [folder_cell setBookmarkCellText:@"test" image:image];
  float folder_cell_x_with_title = ([cell imageRectForBounds:rect]).origin.x;
  float folder_cell_width_with_title = ([cell cellSize]).width;

  EXPECT_EQ(folder_cell_x_without_title, folder_cell_x_with_title);
  EXPECT_EQ(folder_cell_width_without_title, folder_cell_width_with_title);
}

} // namespace
