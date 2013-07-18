/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */
#include <node.h>
#include <iostream>
#include <libssh/server.h>
#include <libssh/keys.h>
#include <libssh/callbacks.h>
#include <string.h>
#include "server.h"
#include "session.h"

namespace nssh {

v8::Persistent<v8::Function> Server::constructor;
v8::Persistent<v8::String> ServerOnConnectionSymbol;

v8::Handle<v8::Value> NewServer (const v8::Arguments& args) {
  v8::HandleScope scope;
  return scope.Close(Server::NewInstance(args));
}

void Server::SocketPollCallback (uv_poll_t* handle, int status, int events) {
  Server* s = static_cast<Server*>(handle->data);

  if (NSSH_DEBUG)
    std::cout << "SocketPollCallback... " << status << ", " << events
      << std::endl;

  ssh_session session = ssh_new();

  int accept = ssh_bind_accept(s->sshbind, session);
  if (accept != SSH_ERROR) {
    if (NSSH_DEBUG) std::cout << "SocketPollCallback:ssh_bind_accept()\n";
    v8::Handle<v8::Object> sess = Session::NewInstance(session);
    //HandleSession(session);
    s->OnConnection(sess);
    node::ObjectWrap::Unwrap<Session>(sess)->Start();
  } else {
    if (NSSH_DEBUG)
      std::cout << "accept failed: " << accept << ", "
        << ssh_get_error(s->sshbind) << std::endl;
  }
}

void IncomingConnectionCallback (ssh_bind sshbind, void *userdata) {
  if (NSSH_DEBUG) std::cout << "IncomingConnectionCallback\n";
}

Server::Server (char *port, char *rsaHostKey, char *dsaHostKey) {
  if (ssh_init()) {
    std::cerr << "ERROR: ssh_init failed";
    return;
  }

  running = false;

  sshbind = ssh_bind_new();
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY, rsaHostKey);
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_DSAKEY, dsaHostKey);
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, port);
  //delete rsaHostKey;
  //delete dsaHostKey;
  //delete port;

  if (NSSH_DEBUG)
    ssh_bind_options_set(
        sshbind
      , SSH_BIND_OPTIONS_LOG_VERBOSITY
      , "SSH_LOG_FUNCTIONS"
    );

  ssh_bind_set_blocking(sshbind, 0);

  if (ssh_bind_listen(sshbind) < 0) {
    //TODO:
    std::cerr << "ERROR: listening to socket: " << ssh_get_error(sshbind)
        << std::endl;
    ssh_bind_free(sshbind);
    exit(-1); // eww... fix this
  }

  assert(ssh_bind_get_fd(sshbind) > 0);

  bindCallbacks = new ssh_bind_callbacks_struct;
  bindCallbacks->incoming_connection = IncomingConnectionCallback;
  ssh_callbacks_init(bindCallbacks);
  ssh_bind_set_callbacks(sshbind, bindCallbacks, 0);

  poll_handle = new uv_poll_t;
  uv_os_sock_t socket = ssh_bind_get_fd(sshbind);
  poll_handle->data = this;
  uv_poll_init_socket(uv_default_loop(), poll_handle, socket);
  uv_poll_start(poll_handle, UV_READABLE, Server::SocketPollCallback);
  running = true;

  if (NSSH_DEBUG) std::cout << "Server::Server done\n";
}

Server::~Server () {
  if (NSSH_DEBUG)
    std::cout << "****************** ~SERVER ******************\n";
}

void Server::Close () {
  if (NSSH_DEBUG)
    std::cerr << "Server::Close()\n";
  if (running) {
    running = false;
    if (poll_handle) {
      uv_poll_stop(poll_handle);
      delete poll_handle;
    }
    if (sshbind)
      ssh_bind_free(sshbind);
    if (bindCallbacks)
      delete bindCallbacks;
  }
}

void Server::OnConnection (v8::Handle<v8::Object> session) {
  v8::HandleScope scope;
  v8::Local<v8::Value> callback = this->handle_->Get(ServerOnConnectionSymbol);
  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = { session };
    callback.As<v8::Function>()->Call(this->handle_, 1, argv);
    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Server::Init () {
  v8::HandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  tpl->SetClassName(v8::String::NewSymbol("Server"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  node::SetPrototypeMethod(tpl, "close", Close);
  constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());
  ServerOnConnectionSymbol = NODE_PSYMBOL("onConnection");
}

v8::Handle<v8::Value> Server::NewInstance (const v8::Arguments& args) {
  v8::HandleScope scope;

  v8::Handle<v8::Value> argv[] = { args[0], args[1], args[2] };
  v8::Local<v8::Object> instance = constructor->NewInstance(3, argv);

  Server *s = ObjectWrap::Unwrap<Server>(instance);
  s->persistentHandle = v8::Persistent<v8::Object>::New(instance);

  return scope.Close(instance);
}

v8::Handle<v8::Value> Server::New (const v8::Arguments& args) {
  v8::HandleScope scope;

  if (args.Length() == 0) {
    NSSH_THROW_RETURN(constructor requires at least a port argument)
  }

  char *port = FromV8String(args[0]);
  char *rsaHostKey = FromV8String(args[1]);
  char *dsaHostKey = FromV8String(args[2]);
  Server* obj = new Server(port, rsaHostKey, dsaHostKey);
  obj->Wrap(args.This());

  return scope.Close(args.This());
}

v8::Handle<v8::Value> Server::Close (const v8::Arguments& args) {
  v8::HandleScope scope;

  Server *s = ObjectWrap::Unwrap<Server>(args.This());
  s->Close();
  s->persistentHandle.Dispose();

  return scope.Close(v8::Undefined());
}

} // namespace nssh
