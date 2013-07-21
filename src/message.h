/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */

#ifndef NSSH_MESSAGE_H
#define NSSH_MESSAGE_H

#include <node.h>
#include <node_buffer.h>
#include <libssh/server.h>
#include <string>

#include "nssh.h"
#include "channel.h"

namespace nssh {

class Message : public node::ObjectWrap {
 public:
  static void Init ();
  static v8::Handle<v8::Object> NewInstance (
      ssh_session session
    , Channel *channel
    , ssh_message message
  );
  static const char* MessageTypeToString (int type);
  static const char* MessageSubtypeToString (int type, int subtype);

  Message ();
  ~Message ();

 private:

  ssh_message message;
  ssh_session session;
  Channel *channel;

  static NAN_METHOD(New);
  static NAN_METHOD(ReplyDefault);
  static NAN_METHOD(ReplyAuthSuccess);
  static NAN_METHOD(ReplySuccess);
  static NAN_METHOD(ComparePublicKey);
  static NAN_METHOD(ScpAccept);
  static NAN_METHOD(SftpAccept);
};

} // namespace nssh

#endif
