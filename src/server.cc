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

static v8::Persistent<v8::FunctionTemplate> server_constructor;

NAN_METHOD(Server::NewInstance) {
  NanScope();

  v8::Local<v8::Object> instance;
  v8::Local<v8::FunctionTemplate> constructorHandle =
      NanPersistentToLocal(server_constructor);
  v8::Handle<v8::Value> argv[] = { args[0], args[1], args[2], args[3] };
  instance = constructorHandle->GetFunction()->NewInstance(4, argv);

  NanReturnValue(instance);
}

void Server::SocketPollCallback (uv_poll_t* handle, int status, int events) {
  NanScope();

  Server* s = static_cast<Server*>(handle->data);

  if (NSSH_DEBUG)
    std::cout << "SocketPollCallback... " << status << ", " << events
      << std::endl;

  ssh_session session = ssh_new();

  int accept = ssh_bind_accept(s->sshbind, session);
  if (accept != SSH_ERROR) {
    if (NSSH_DEBUG) std::cout << "SocketPollCallback:ssh_bind_accept()\n";
    v8::Handle<v8::Object> sess = Session::NewInstance(session);
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

Server::Server (char *port, char *rsaHostKey, char *dsaHostKey, char *banner) {
  running = false;

  if (ssh_init()) {
    std::cerr << "ERROR: ssh_init failed";
    return;
  }

  this->port = port;

  if (NSSH_DEBUG)
    std::cerr << "Server::Server running=false " << port << "\n";

  sshbind = ssh_bind_new();
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY, rsaHostKey);
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_DSAKEY, dsaHostKey);
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, port);
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BANNER, banner);
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

  /*
  bindCallbacks = new ssh_bind_callbacks_struct;
  bindCallbacks->incoming_connection = IncomingConnectionCallback;
  ssh_callbacks_init(bindCallbacks);
  ssh_bind_set_callbacks(sshbind, bindCallbacks, 0);
  */

  poll_handle = new uv_poll_t;
  uv_os_sock_t socket = ssh_bind_get_fd(sshbind);
  poll_handle->data = this;
  uv_loop_t *loop = uv_default_loop();
  uv_poll_init_socket(loop, poll_handle, socket);
  uv_poll_start(poll_handle, UV_READABLE, Server::SocketPollCallback);
  running = true;
  if (NSSH_DEBUG)
    std::cerr << "Server::Server running=true " << port << "\n";

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
    if (NSSH_DEBUG)
      std::cerr << "Server::Close running=false " << port << "\n";
    //std::cerr << "+++ Server::Close running=false " << port << ", " << poll_handle->loop << "\n";
    uv_poll_stop(poll_handle);
    delete poll_handle;
    ssh_bind_free(sshbind);
    if (NSSH_DEBUG)
      std::cerr << "Server::Close ssh_bind_free\n";

    ssh_finalize();
    /*
    if (bindCallbacks)
      delete bindCallbacks;
      */
  }
}

void Server::OnConnection (v8::Handle<v8::Object> session) {
  NanScope();

  v8::Local<v8::Value> callback = NanObjectWrapHandle(this)
      ->Get(NanSymbol("onConnection"));
  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = { session };
    callback.As<v8::Function>()->Call(NanObjectWrapHandle(this), 1, argv);
    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Server::Init () {
  NanScope();

  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  NanAssignPersistent(v8::FunctionTemplate, server_constructor, tpl);
  tpl->SetClassName(NanSymbol("Server"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);
}

NAN_METHOD(Server::New) {
  NanScope();

  if (args.Length() == 0)
    return NanThrowError("constructor requires at least a port argument");

  char *port = NanFromV8String(
      args[0]->ToString()
    , Nan::UTF8
    , NULL
    , NULL
    , 0
    , v8::String::NO_OPTIONS
  );
  char *rsaHostKey = NanFromV8String(
      args[1].As<v8::Object>()
    , Nan::UTF8
    , NULL
    , NULL
    , 0
    , v8::String::NO_OPTIONS
  );
  char *dsaHostKey = NanFromV8String(
      args[2].As<v8::Object>()
    , Nan::UTF8
    , NULL
    , NULL
    , 0
    , v8::String::NO_OPTIONS
  );

  char *banner = NanFromV8String(
      args[3].As<v8::Object>()
    , Nan::UTF8
    , NULL
    , NULL
    , 0
    , v8::String::NO_OPTIONS
  );

  Server* obj = new Server(port, rsaHostKey, dsaHostKey, banner);
  obj->Wrap(args.This());

  NanReturnValue(args.This());
}

NAN_METHOD(Server::Close) {
  NanScope();

  Server *s = ObjectWrap::Unwrap<Server>(args.This());
  s->Close();
  s->persistentHandle.Dispose();

  NanReturnValue(v8::Undefined());
}

} // namespace nssh
