/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */

#include <node.h>

#include "nssh.h"
#include "server.h"
#include "session.h"
#include "message.h"
#include "sftp_message.h"

namespace nssh {

void Init (v8::Handle<v8::Object> target) {
  Server::Init();
  Session::Init();
  Channel::Init();
  Message::Init();
  SftpMessage::Init();

  v8::Local<v8::Function> Server
      = v8::FunctionTemplate::New(Server::NewInstance)->GetFunction();
  target->Set(NanSymbol("Server"), Server);
}

NODE_MODULE(ssh, Init)

} // namespace nssh
