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
  static v8::Persistent<v8::Function> constructor;

  ssh_message message;
  ssh_session session;
  Channel *channel;
  const char *type;
  const char *subtype;

  NSSH_V8_METHOD( New              )
  NSSH_V8_METHOD( ReplyDefault     )
  NSSH_V8_METHOD( ReplyAuthSuccess )
  NSSH_V8_METHOD( ReplySuccess     )
  NSSH_V8_METHOD( ComparePublicKey )
  NSSH_V8_METHOD( ScpAccept        )
  NSSH_V8_METHOD( SftpAccept       )
};

} // namespace nssh

#endif
