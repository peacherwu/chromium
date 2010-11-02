// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class is an implementation of the ChromotingView using Pepper devices
// as the backing stores.  The public APIs to this class are thread-safe.
// Calls will dispatch any interaction with the pepper API onto the pepper
// main thread.
//
// TODO(ajwong): We need to better understand the threading semantics of this
// class.  Currently, we're just going to always run everything on the pepper
// main thread.  Is this smart?

#ifndef REMOTING_CLIENT_PLUGIN_PEPPER_VIEW_H_
#define REMOTING_CLIENT_PLUGIN_PEPPER_VIEW_H_

#include "base/scoped_ptr.h"
#include "base/task.h"
#include "media/base/video_frame.h"
#include "remoting/client/chromoting_view.h"
#include "remoting/client/frame_consumer.h"
#include "remoting/client/rectangle_update_decoder.h"
#include "third_party/ppapi/cpp/graphics_2d.h"

namespace remoting {

class ChromotingInstance;
class ClientContext;

class PepperView : public ChromotingView,
                   public FrameConsumer {
 public:
  // Constructs a PepperView that draws to the |rendering_device|. The
  // |rendering_device| instance must outlive this class.
  PepperView(ChromotingInstance* instance, ClientContext* context);
  virtual ~PepperView();

  // ChromotingView implementation.
  virtual bool Initialize();
  virtual void TearDown();
  virtual void Paint();
  virtual void SetSolidFill(uint32 color);
  virtual void UnsetSolidFill();
  virtual void SetConnectionState(ConnectionState state);
  virtual void SetViewport(int x, int y, int width, int height);

  // FrameConsumer implementation.
  virtual void AllocateFrame(media::VideoFrame::Format format,
                             size_t width,
                             size_t height,
                             base::TimeDelta timestamp,
                             base::TimeDelta duration,
                             scoped_refptr<media::VideoFrame>* frame_out,
                             Task* done);
  virtual void ReleaseFrame(media::VideoFrame* frame);
  virtual void OnPartialFrameOutput(media::VideoFrame* frame,
                                    UpdatedRects* rects,
                                    Task* done);

 private:
  void OnPaintDone();
  void PaintFrame(media::VideoFrame* frame, UpdatedRects* rects);

  // Reference to the creating plugin instance. Needed for interacting with
  // pepper.  Marking explciitly as const since it must be initialized at
  // object creation, and never change.
  ChromotingInstance* const instance_;

  // Context should be constant for the lifetime of the plugin.
  ClientContext* const context_;

  pp::Graphics2D graphics2d_;

  int viewport_x_;
  int viewport_y_;
  int viewport_width_;
  int viewport_height_;

  bool is_static_fill_;
  uint32 static_fill_color_;

  DISALLOW_COPY_AND_ASSIGN(PepperView);
};

}  // namespace remoting

DISABLE_RUNNABLE_METHOD_REFCOUNT(remoting::PepperView);

#endif  // REMOTING_CLIENT_PLUGIN_PEPPER_VIEW_H_
