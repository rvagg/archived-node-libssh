/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */

#ifndef NSSH_CHANNEL_H
#define NSSH_CHANNEL_H

#include <node.h>
#include <node_buffer.h>
#include <libssh/server.h>
#include <libssh/sftp.h>
#include <libssh/callbacks.h>
#include <string>
#include <nan.h>

#include "nssh.h"

namespace nssh {

class Channel : public node::ObjectWrap {
 public:
  typedef void (*ChannelClosedCallback) (Channel *channel, void *userData);

  static void Init ();
  static v8::Handle<v8::Object> NewInstance (
      ssh_session session
    , ssh_channel channel
    , ChannelClosedCallback channelClosedCallback
    , void *callbackUserData
  );

  Channel ();
  ~Channel ();

  void Setup ();
  void CloseChannel ();
  void SetSftp (sftp_session sftp);

  ssh_channel channel;
  int myid;

  void OnError (std::string error);
  void OnMessage (v8::Handle<v8::Object> message);
  void OnSftpMessage (v8::Handle<v8::Object> message);
  void OnData (const char *data, int length);
  void OnClose ();
  bool IsChannel (ssh_channel);
  bool TryRead ();

 private:
  static void SocketPollCallback(uv_poll_t* handle, int status, int events);

  void SetupCallbacks (bool includeData);

  sftp_session sftp;
  bool sftpinit;
  ChannelClosedCallback channelClosedCallback;
  void *callbackUserData;
  ssh_session session;
  ssh_channel_callbacks_struct *callbacks;
  bool closed;

  static NAN_METHOD(New);
  static NAN_METHOD(Start);
  static NAN_METHOD(WriteData);
  static NAN_METHOD(SendExitStatus);
  static NAN_METHOD(Close);
  static NAN_METHOD(SendEof);

};

} // namespace nssh

#endif
