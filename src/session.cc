/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */
#include <node.h>
#include <iostream>
#include <libssh/server.h>
#include <libssh/keys.h>
#include <libssh/socket.h>
#include <libssh/poll.h>
#include <string.h>
#include "session.h"
#include "message.h"

namespace nssh {

v8::Persistent<v8::Function> Session::constructor;
v8::Persistent<v8::String> SessionOnMessageSymbol;
v8::Persistent<v8::String> SessionOnNewChannelSymbol;

void Session::OnError (std::string err) {
  //TODO:
  std::cerr << "ERROR: " << err << std::endl;
}

void Session::OnMessage (v8::Handle<v8::Object> message) {
  v8::HandleScope scope;

  v8::Local<v8::Value> callback = this->handle_->Get(SessionOnMessageSymbol);

  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = { message };
    callback.As<v8::Function>()->Call(this->handle_, 1, argv);

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Session::OnNewChannel (v8::Handle<v8::Object> channel) {
  v8::HandleScope scope;

  v8::Local<v8::Value> callback = this->handle_->Get(SessionOnNewChannelSymbol);

  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = { channel };
    callback.As<v8::Function>()->Call(this->handle_, 1, argv);

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Session::ChannelClosedCallback (Channel *channel, void *userData) {
  Session* s = static_cast<Session*>(userData);

  if (NSSH_DEBUG)
    std::cout << "ChannelClosedCallback!\n";

  std::vector<Channel*>::iterator it = s->channels.begin();
  while (it != s->channels.end()) {
    if ((*it)->IsChannel(channel->channel)) {
      it = s->channels.erase(it);
      if (NSSH_DEBUG)
        std::cout << "Found and removed channel from list\n";
    }
    else
      ++it;
  }
  if (s->channels.size() == 0)
    s->Close();
}

void Session::SocketPollCallback (uv_poll_t* handle, int status, int events) {
  Session* s = static_cast<Session*>(handle->data);

  if (NSSH_DEBUG) {
    std::cout << "SocketPollCallback... " << status << ", " << events
      << ", " << ssh_get_status(s->session)
      << std::endl;
  }

  // doesn't suit async handling: ssh_execute_message_callbacks(s->session);

  ssh_message message;
  int type;
  int subtype;

  while (true) {
    if (NSSH_DEBUG)
      std::cout << "checking for message...\n";

    message = ssh_message_get(s->session);

    if (NSSH_DEBUG)
      std::cout << "message loop " << (!message) << std::endl;

    if (!message)
      break;

    type = ssh_message_type(message);
    subtype = ssh_message_subtype(message);

    if (NSSH_DEBUG)
      std::cout << "message, type = " << type << ", subtype = " << subtype
        << std::endl;

    if (type == SSH_REQUEST_CHANNEL_OPEN && subtype == SSH_CHANNEL_SESSION) {

      if (NSSH_DEBUG)
        std::cout << "New Channel\n";

      v8::Handle<v8::Object> channel = Channel::NewInstance(
          s->session
        , ssh_message_channel_request_open_reply_accept(message)
        , s->ChannelClosedCallback
        , s
      );

      s->channels.push_back(
          node::ObjectWrap::Unwrap<Channel>(channel));
      s->OnNewChannel(channel);

      ssh_message_free(message);
    } else {
      if (type == SSH_REQUEST_CHANNEL && subtype) {
        for (std::vector<Channel*>::iterator it = s->channels.begin()
              ; it != s->channels.end(); ++it) {
          if ((*it)->IsChannel(ssh_message_channel_request_channel(message))) {
            (*it)->OnMessage(Message::NewInstance(s->session, *it, message));
            break;
          }
        }
        ssh_message_free(message);
      } else {
        v8::Handle<v8::Object> mess =
            Message::NewInstance(s->session, NULL, message);
        s->OnMessage(mess);
        // freed on ~Message()
      }
    }
  }

  bool channelData = false;
  for (std::vector<Channel*>::iterator it = s->channels.begin()
        ; it != s->channels.end(); ++it) {
    if ((*it)->TryRead()) {
      channelData = true;
      break;
    }
  }

  if (NSSH_DEBUG)
    std::cout << "Channel Data: " << (channelData ? "yes" : "no") << std::endl;
}

Session::Session () {
  keysExchanged = true;
}

Session::~Session () {
  Close();
  ssh_free(session);
  delete callbacks;
}

void Session::Close () {
  uv_poll_stop(poll_handle);
  //delete poll_handle;
  ssh_set_callbacks(session, 0);
  ssh_set_message_callback(session, 0, 0);
  ssh_disconnect(session);
  if (NSSH_DEBUG)
    std::cout << "Stopped polling session, " << channels.size() << " channels open\n";
}

// a client callback (I think)
int SessionAuthCallback (const char *prompt, char *buf, size_t len,
    int echo, int verify, void *userdata) {

  if (NSSH_DEBUG)
    std::cout << "SessionAuthCallback\n";
  return 0;
}

void SessionLogCallback (ssh_session session, int priority,
    const char *message, void *userdata) {

  if (NSSH_DEBUG)
    std::cout << "SessionLogCallback: " << priority << ": " << message
      << std::endl;
}

// not sure if this gets used
void SessionGlobalRequestCallback (ssh_session session, ssh_message message,
    void *userdata) {

  if (NSSH_DEBUG)
    std::cout << "SessionGlobalRequestCallback!\n";
}

// not used as far as I can tell
int SessionMessageCallback (ssh_session session, ssh_message msg, void *data) {
  if (NSSH_DEBUG)
    std::cout << "SessionMessageCallback!\n";
  return 1;
}

void SessionStatusCallback (void *userdata, float status) {
  if (NSSH_DEBUG)
    std::cout << "SessionStatusCallback: " << status << std::endl;
}


void Session::Start () {
  callbacks = new ssh_callbacks_struct;
  callbacks->auth_function = SessionAuthCallback;
  callbacks->log_function = SessionLogCallback;
  callbacks->global_request_function = SessionGlobalRequestCallback;
  callbacks->connect_status_function = SessionStatusCallback;
  callbacks->userdata = this;
  ssh_callbacks_init(callbacks);
  ssh_set_callbacks(session, callbacks);
  ssh_set_message_callback(session, SessionMessageCallback, this);

  ssh_options_set(session, SSH_OPTIONS_TIMEOUT, "0");
  ssh_options_set(session, SSH_OPTIONS_TIMEOUT_USEC, "1");
  ssh_set_blocking(session, 0);

  //TODO: do this async
  if (ssh_handle_key_exchange(session)) {
    std::string err("Key exchange error: ");
    err.append(ssh_get_error(session));
    OnError(err);
  } else {
    keysExchanged = true;
    if (NSSH_DEBUG)
      std::cout << "keysExchanged\n";
  }

  poll_handle = new uv_poll_t;
  uv_os_sock_t socket = ssh_get_fd(session);
  poll_handle->data = this;
  uv_poll_init_socket(uv_default_loop(), poll_handle, socket);
  uv_poll_start(poll_handle, UV_READABLE, SocketPollCallback);

  if (NSSH_DEBUG)
    std::cout << "polling started\n";
}

void Session::Init () {
  v8::HandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  tpl->SetClassName(v8::String::NewSymbol("Session"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  node::SetPrototypeMethod(tpl, "close", Close);
  constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());
  SessionOnMessageSymbol = NODE_PSYMBOL("onMessage");
  SessionOnNewChannelSymbol = NODE_PSYMBOL("onNewChannel");
}

v8::Handle<v8::Object> Session::NewInstance (ssh_session session) {
  v8::HandleScope scope;

  v8::Local<v8::Object> instance = constructor->NewInstance(0, NULL);
  Session *s = ObjectWrap::Unwrap<Session>(instance);
  s->session = session;
  s->persistentHandle = v8::Persistent<v8::Object>::New(instance);

  return scope.Close(instance);
}

v8::Handle<v8::Value> Session::New (const v8::Arguments& args) {
  v8::HandleScope scope;

  Session* obj = new Session();
  obj->Wrap(args.This());
  if (NSSH_DEBUG)
    std::cout << "Session::New()" << std::endl;

  return scope.Close(args.This());
}

v8::Handle<v8::Value> Session::Close (const v8::Arguments& args) {
  v8::HandleScope scope;

  Session *s = ObjectWrap::Unwrap<Session>(args.This());
  s->Close();
  s->persistentHandle.Dispose();

  return scope.Close(v8::Undefined());
}

} // namespace nssh
