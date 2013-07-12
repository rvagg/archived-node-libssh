/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */

#ifndef NSSH_SESSION_H
#define NSSH_SESSION_H

#include <node.h>
#include <node_buffer.h>
#include <libssh/server.h>
#include <libssh/poll.h>
#include <string>
#include <vector>

#include "nssh.h"
#include "channel.h"

namespace nssh {

class Session : public node::ObjectWrap {
 public:
  static void Init ();
  static v8::Handle<v8::Object> NewInstance (ssh_session session);

  Session ();
  ~Session ();

  void Start ();
  void Close ();
  void OnMessage (v8::Handle<v8::Object> message);
  void OnNewChannel (v8::Handle<v8::Object> channel);
  void OnError (std::string error);

 private:
  static v8::Persistent<v8::Function> constructor;
  static void SocketPollCallback (uv_poll_t* handle, int status, int events);
  static void ChannelClosedCallback (Channel *channel, void *user);

  bool keysExchanged;
  ssh_session session;
  uv_poll_t *poll_handle;
  ssh_callbacks_struct *callbacks;
  v8::Persistent<v8::Object> persistentHandle;

  std::vector<Channel*> channels;

  NSSH_V8_METHOD( New )
  NSSH_V8_METHOD( Close )
};

} // namespace nssh

#endif
