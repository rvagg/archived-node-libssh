/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */

#ifndef NSSH_SERVER_H
#define NSSH_SERVER_H

#include <node.h>
#include <node_buffer.h>

#include <libssh/server.h>

#include "nssh.h"

namespace nssh {

v8::Handle<v8::Value> NewServer (const v8::Arguments& args);

class Server : public node::ObjectWrap {
 public:
  static void Init ();
  static v8::Handle<v8::Value> NewInstance (const v8::Arguments& args);

  Server (char *port, char *rsaHostKey, char *dsaHostKey);
  ~Server ();

  void OnConnection (v8::Handle<v8::Object> session);
  void Close ();

 private:
  static v8::Persistent<v8::Function> constructor;
  static void SocketPollCallback (uv_poll_t* handle, int status, int events);

  ssh_bind sshbind;
  uv_poll_t *poll_handle;
  ssh_bind_callbacks_struct *bindCallbacks;
  v8::Persistent<v8::Object> persistentHandle;

  NSSH_V8_METHOD( New   )
  NSSH_V8_METHOD( Close )
};

} // namespace nssh

#endif
