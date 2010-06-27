// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides the embedder's side of random webkit glue functions.

#include "build/build_config.h"

#include <vector>

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "app/clipboard/clipboard.h"
#include "app/clipboard/scoped_clipboard_writer.h"
#include "app/resource_bundle.h"
#include "base/file_version_info.h"
#include "base/ref_counted.h"
#include "base/string_util.h"
#include "chrome/app/chrome_version_info.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/socket_stream_dispatcher.h"
#include "chrome/common/url_constants.h"
#include "chrome/plugin/npobject_util.h"
#include "chrome/renderer/net/renderer_net_predictor.h"
#include "chrome/renderer/render_process.h"
#include "chrome/renderer/render_thread.h"
#include "googleurl/src/url_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/WebKit/WebKit/chromium/public/WebKit.h"
#include "third_party/WebKit/WebKit/chromium/public/WebKitClient.h"
#include "third_party/WebKit/WebKit/chromium/public/WebString.h"
#include "webkit/glue/scoped_clipboard_writer_glue.h"
#include "webkit/glue/webkit_glue.h"
#include "webkit/glue/websocketstreamhandle_bridge.h"

#if defined(OS_WIN)
#include <strsafe.h>  // note: per msdn docs, this must *follow* other includes
#endif

template <typename T, size_t stack_capacity>
class ResizableStackArray {
 public:
  ResizableStackArray()
      : cur_buffer_(stack_buffer_), cur_capacity_(stack_capacity) {
  }
  ~ResizableStackArray() {
    FreeHeap();
  }

  T* get() const {
    return cur_buffer_;
  }

  T& operator[](size_t i) {
    return cur_buffer_[i];
  }

  size_t capacity() const {
    return cur_capacity_;
  }

  void Resize(size_t new_size) {
    if (new_size < cur_capacity_)
      return;  // already big enough
    FreeHeap();
    cur_capacity_ = new_size;
    cur_buffer_ = new T[new_size];
  }

 private:
  // Resets the heap buffer, if any
  void FreeHeap() {
    if (cur_buffer_ != stack_buffer_) {
      delete[] cur_buffer_;
      cur_buffer_ = stack_buffer_;
      cur_capacity_ = stack_capacity;
    }
  }

  T stack_buffer_[stack_capacity];
  T* cur_buffer_;
  size_t cur_capacity_;
};

// This definition of WriteBitmapFromPixels uses shared memory to communicate
// across processes.
void ScopedClipboardWriterGlue::WriteBitmapFromPixels(const void* pixels,
                                                      const gfx::Size& size) {
  // Do not try to write a bitmap more than once
  if (shared_buf_)
    return;

  uint32 buf_size = 4 * size.width() * size.height();

  // Allocate a shared memory buffer to hold the bitmap bits.
#if defined(OS_POSIX)
  // On POSIX, we need to ask the browser to create the shared memory for us,
  // since this is blocked by the sandbox.
  base::SharedMemoryHandle shared_mem_handle;
  ViewHostMsg_AllocateSharedMemoryBuffer *msg =
      new ViewHostMsg_AllocateSharedMemoryBuffer(buf_size,
                                                 &shared_mem_handle);
  if (RenderThread::current()->Send(msg)) {
    if (base::SharedMemory::IsHandleValid(shared_mem_handle)) {
      shared_buf_ = new base::SharedMemory(shared_mem_handle, false);
      if (!shared_buf_ || !shared_buf_->Map(buf_size)) {
        NOTREACHED() << "Map failed";
        return;
      }
    } else {
      NOTREACHED() << "Browser failed to allocate shared memory";
      return;
    }
  } else {
    NOTREACHED() << "Browser allocation request message failed";
    return;
  }
#else  // !OS_POSIX
  shared_buf_ = new base::SharedMemory;
  const bool created = shared_buf_ && shared_buf_->Create(
      L"", false /* read write */, true /* open existing */, buf_size);
  if (!shared_buf_ || !created || !shared_buf_->Map(buf_size)) {
    NOTREACHED();
    return;
  }
#endif

  // Copy the bits into shared memory
  memcpy(shared_buf_->memory(), pixels, buf_size);
  shared_buf_->Unmap();

  Clipboard::ObjectMapParam size_param;
  const char* size_data = reinterpret_cast<const char*>(&size);
  for (size_t i = 0; i < sizeof(gfx::Size); ++i)
    size_param.push_back(size_data[i]);

  Clipboard::ObjectMapParams params;

  // The first parameter is replaced on the receiving end with a pointer to
  // a shared memory object containing the bitmap. We reserve space for it here.
  Clipboard::ObjectMapParam place_holder_param;
  params.push_back(place_holder_param);
  params.push_back(size_param);
  objects_[Clipboard::CBF_SMBITMAP] = params;
}

// Define a destructor that makes IPCs to flush the contents to the
// system clipboard.
ScopedClipboardWriterGlue::~ScopedClipboardWriterGlue() {
  if (objects_.empty())
    return;

  if (shared_buf_) {
    RenderThread::current()->Send(
        new ViewHostMsg_ClipboardWriteObjectsSync(objects_,
                shared_buf_->handle()));
    delete shared_buf_;
    return;
  }

  RenderThread::current()->Send(
      new ViewHostMsg_ClipboardWriteObjectsAsync(objects_));
}

namespace webkit_glue {

void PrecacheUrl(const wchar_t* url, int url_length) {
  // TBD: jar: Need implementation that loads the targetted URL into our cache.
  // For now, at least prefetch DNS lookup
  std::string url_string;
  WideToUTF8(url, url_length, &url_string);
  const std::string host = GURL(url_string).host();
  if (!host.empty())
    DnsPrefetchCString(host.data(), host.length());
}

void AppendToLog(const char* file, int line, const char* msg) {
  logging::LogMessage(file, line).stream() << msg;
}

base::StringPiece GetDataResource(int resource_id) {
  return ResourceBundle::GetSharedInstance().GetRawDataResource(resource_id);
}

#if defined(OS_WIN)
HCURSOR LoadCursor(int cursor_id) {
  return ResourceBundle::GetSharedInstance().LoadCursor(cursor_id);
}
#endif

// Clipboard glue

Clipboard* ClipboardGetClipboard() {
  return NULL;
}

bool ClipboardIsFormatAvailable(const Clipboard::FormatType& format,
                                Clipboard::Buffer buffer) {
  bool result;
  RenderThread::current()->Send(
      new ViewHostMsg_ClipboardIsFormatAvailable(format, buffer, &result));
  return result;
}

void ClipboardReadText(Clipboard::Buffer buffer, string16* result) {
  RenderThread::current()->Send(new ViewHostMsg_ClipboardReadText(buffer,
                                                                  result));
}

void ClipboardReadAsciiText(Clipboard::Buffer buffer, std::string* result) {
  RenderThread::current()->Send(new ViewHostMsg_ClipboardReadAsciiText(buffer,
                                                                       result));
}

void ClipboardReadHTML(Clipboard::Buffer buffer, string16* markup, GURL* url) {
  RenderThread::current()->Send(new ViewHostMsg_ClipboardReadHTML(buffer,
                                                                  markup, url));
}

void GetPlugins(bool refresh, std::vector<WebPluginInfo>* plugins) {
  if (!RenderThread::current()->plugin_refresh_allowed())
    refresh = false;
  RenderThread::current()->Send(new ViewHostMsg_GetPlugins(refresh, plugins));
}

bool IsProtocolSupportedForMedia(const GURL& url) {
  // If new protocol is to be added here, we need to make sure the response is
  // validated accordingly in the media engine.
  if (url.SchemeIsFile() || url.SchemeIs(chrome::kHttpScheme) ||
      url.SchemeIs(chrome::kHttpsScheme) ||
      url.SchemeIs(chrome::kDataScheme) ||
      url.SchemeIs(chrome::kExtensionScheme))
    return true;
  return false;
}

// static factory function
ResourceLoaderBridge* ResourceLoaderBridge::Create(
    const ResourceLoaderBridge::RequestInfo& request_info) {
  return ChildThread::current()->CreateBridge(request_info, -1, -1);
}

// static factory function
WebSocketStreamHandleBridge* WebSocketStreamHandleBridge::Create(
    WebKit::WebSocketStreamHandle* handle,
    WebSocketStreamHandleDelegate* delegate) {
  SocketStreamDispatcher* dispatcher =
      ChildThread::current()->socket_stream_dispatcher();
  return dispatcher->CreateBridge(handle, delegate);
}

void NotifyCacheStats() {
  // Update the browser about our cache
  // NOTE: Since this can be called from the plugin process, we might not have
  // a RenderThread.  Do nothing in that case.
  if (RenderThread::current())
    RenderThread::current()->InformHostOfCacheStatsLater();
}

void CloseCurrentConnections() {
  RenderThread::current()->CloseCurrentConnections();
}

void SetCacheMode(bool enabled) {
  RenderThread::current()->SetCacheMode(enabled);
}

void ClearCache() {
  RenderThread::current()->ClearCache();
}

std::string GetProductVersion() {
  scoped_ptr<FileVersionInfo> version_info(
      chrome_app::GetChromeVersionInfo());
  std::string product("Chrome/");
  product += version_info.get() ? WideToASCII(version_info->product_version())
                                : "0.0.0.0";
  return product;
}

}  // namespace webkit_glue
