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
  static v8::Persistent<v8::Function> constructor;

  sftp_client_message message;
  ssh_session session;
  Channel *channel;

  NSSH_V8_METHOD( New         )
  NSSH_V8_METHOD( ReplyName   )
  NSSH_V8_METHOD( ReplyNames  )
  NSSH_V8_METHOD( ReplyAttr   )
  NSSH_V8_METHOD( ReplyHandle )
  NSSH_V8_METHOD( ReplyStatus )
  NSSH_V8_METHOD( ReplyData   )
};

} // namespace nssh

#endif
