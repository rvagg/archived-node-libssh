/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */

#ifndef NSSH_SFTPMESSAGE_H
#define NSSH_SFTPMESSAGE_H

#include <node.h>
#include <node_buffer.h>
#include <libssh/server.h>
#include <string>

#include "nssh.h"
#include "channel.h"

namespace nssh {

class SftpMessage : public node::ObjectWrap {
 public:
  static void Init ();
  static v8::Handle<v8::Object> NewInstance (
      ssh_session session
    , Channel *channel
    , sftp_client_message message
  );
  static const char* MessageTypeToString (int type);

  SftpMessage ();
  ~SftpMessage ();

 private:
  sftp_client_message message;
  ssh_session session;
  Channel *channel;

  static NAN_METHOD(New);
  static NAN_METHOD(ReplyName);
  static NAN_METHOD(ReplyNames);
  static NAN_METHOD(ReplyAttr);
  static NAN_METHOD(ReplyHandle);
  static NAN_METHOD(ReplyStatus);
  static NAN_METHOD(ReplyData);
};

} // namespace nssh

#endif
