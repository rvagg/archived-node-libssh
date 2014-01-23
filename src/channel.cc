/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */
#include <node.h>
#include <nan.h>
#include <node_buffer.h>
#include <iostream>
#include <libssh/server.h>
#include <libssh/keys.h>
#include <libssh/callbacks.h>
#include <libssh/channels.h>
#include <string.h>
#include "channel.h"
#include "sftp_message.h"

namespace nssh {

v8::Persistent<v8::FunctionTemplate> channel_constructor;

static int ids = 0;

Channel::Channel () {
  sftp = NULL;
  sftpinit = false;
  callbacks = NULL;
  closed = false;
  myid = ids++;
  if (NSSH_DEBUG)
    std::cout << "Channel::Channel! " << myid << "\n";
}

Channel::~Channel () {
}

void Channel::SetSftp (sftp_session sftp) {
  this->sftp = sftp;
}

// not used, doesn't work so well so we use uv polling instead and process
// messages on our own
int ChannelDataCallback (
      ssh_session session
    , ssh_channel channel
    , void *data
    , uint32_t len
    , int is_stderr
    , void *userdata
  ) {

  Channel* c = static_cast<Channel*>(userdata);
  if (NSSH_DEBUG)
    std::cout << "ChannelDataCallback! " << std::string((char *)data, len)
      << std::endl;

  c->OnData((char *)data, len);
  return 1;
}

void ChannelEofCallback (
      ssh_session session
    , ssh_channel channel
    , void *userdata) {

  Channel* c = static_cast<Channel*>(userdata);
  if (NSSH_DEBUG)
    std::cout << "ChannelEofCallback!\n";
  // try one last read!
  c->CloseChannel();
}

void ChannelCloseCallback (
      ssh_session session
    , ssh_channel channel
    , void *userdata) {

  if (NSSH_DEBUG)
    std::cout << "ChannelCloseCallback!\n";
  Channel* c = static_cast<Channel*>(userdata);
  // try one last read!
  c->CloseChannel();
}

void ChannelSignalCallback (
      ssh_session session
    , ssh_channel channel
    , const char* signal
    , void *userdata) {

//  Channel* c = static_cast<Channel*>(userdata);
  if (NSSH_DEBUG)
    std::cout << "ChannelSignalCallback!\n";
}

void Channel::SetupCallbacks (bool includeData) {
  if (callbacks)
    delete callbacks;
  if (NSSH_DEBUG)
    std::cout << "SetupCallbacks()\n";

  callbacks = new ssh_channel_callbacks_struct;
  callbacks->channel_data_function = 0; // See note at ChannelCloseCallback
  callbacks->channel_eof_function = ChannelEofCallback;
  callbacks->channel_close_function = ChannelCloseCallback;
  callbacks->channel_signal_function = ChannelSignalCallback;
  callbacks->userdata = this;
  ssh_callbacks_init(callbacks);
  ssh_set_channel_callbacks(channel, callbacks);
}

void Channel::CloseChannel () {
  if (!closed) {
    TryRead(); // one last time
    if (NSSH_DEBUG)
      std::cout << "CloseChannel, closed = true " << myid << "\n";
    if (NSSH_DEBUG)
      std::cout << "ssh_channel_close()\n";
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    if (channelClosedCallback)
      channelClosedCallback(this, callbackUserData);
    //TryRead(); // not really a read, just flush the msg buffer
               // otherwise the channel may just hang
    closed = true;
    OnClose();
  }
}

bool Channel::TryRead () {
  NanScope();

  if (NSSH_DEBUG)
    std::cout << "TryRead closed=" << (closed ? "true" : "false") << " " << myid << std::endl;
  if (closed)
    return false;

  sftp_client_message sftpmessage;
  bool read = false;

  if (sftp) {
    while (true) {
      if (!sftpinit) {
        int rc = sftp_server_init(sftp);
        if (rc) {
          if (NSSH_DEBUG)
            std::cout << "Error sftp_server_init error " << rc << ": " << sftp_get_error(sftp) << std::endl;
          return false;
        } else {
          sftpinit = true;
          if (NSSH_DEBUG)
            std::cout << "sftp_server_init() successful\n";
        }
      }

      if (NSSH_DEBUG)
        std::cout << "sftp=true\n";

      sftpmessage = sftp_get_client_message(sftp);
      if (sftpmessage) {
        read = true;
        if (NSSH_DEBUG)
          std::cout << "TryRead sftp Message " << sftpmessage << std::endl;
        v8::Handle<v8::Object> mess = SftpMessage::NewInstance(session, this, sftpmessage);
        OnSftpMessage(mess);
      } else
        break;
    }
    return read;
  }

  int len;
  do {
    char buf[1024];
    len = ssh_channel_read_nonblocking(channel, buf, sizeof(buf), 0);
    if (len > 0) {
      read = true;
      if (NSSH_DEBUG)
        std::cout << "Read buf = " << std::string(buf, len) << std::endl;
      OnData(buf, len);
    } else
      break;
  } while (true);

  if (NSSH_DEBUG)
    std::cout << "Channel::TryRead len=" << len << std::endl;
  return read;
}

bool Channel::IsChannel (ssh_channel channel) {
  return this->channel == channel;
}

void Channel::OnMessage (v8::Handle<v8::Object> mess) {
  NanScope();

  if (NSSH_DEBUG)
    std::cout << "Channel::OnMessage\n";

  v8::Local<v8::Value> callback = NanObjectWrapHandle(this)
      ->Get(NanSymbol("onMessage"));

  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = { mess };
    callback.As<v8::Function>()->Call(NanObjectWrapHandle(this), 1, argv);

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Channel::OnSftpMessage (v8::Handle<v8::Object> mess) {
  NanScope();

  if (NSSH_DEBUG)
    std::cout << "Channel::OnSftpMessage\n";

  v8::Local<v8::Value> callback = NanObjectWrapHandle(this)
      ->Get(NanSymbol("onSftpMessage"));

  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = { mess };
    callback.As<v8::Function>()->Call(NanObjectWrapHandle(this), 1, argv);

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Channel::OnData (const char *data, int length) {
  NanScope();

  v8::Local<v8::Value> callback = NanObjectWrapHandle(this)
      ->Get(NanSymbol("onData"));

  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = {
      NanNewBufferHandle((char *)data, length)
    };

    callback.As<v8::Function>()->Call(NanObjectWrapHandle(this), 1, argv);

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Channel::OnClose () {
  NanScope();

  v8::Local<v8::Value> callback = NanObjectWrapHandle(this)
      ->Get(NanSymbol("onClose"));

  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    callback.As<v8::Function>()->Call(NanObjectWrapHandle(this), 0, NULL);
    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Channel::Init () {
  NanScope();

  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  NanAssignPersistent(v8::FunctionTemplate, channel_constructor, tpl);
  tpl->SetClassName(NanSymbol("Channel"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  NODE_SET_PROTOTYPE_METHOD(tpl, "start", Start);
  NODE_SET_PROTOTYPE_METHOD(tpl, "writeData", WriteData);
  NODE_SET_PROTOTYPE_METHOD(tpl, "sendEof", SendEof);
  NODE_SET_PROTOTYPE_METHOD(tpl, "sendExitStatus", SendExitStatus);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);
}

v8::Handle<v8::Object> Channel::NewInstance (
      ssh_session session
    , ssh_channel channel
    , ChannelClosedCallback channelClosedCallback
    , void *callbackUserData
  ) {

  NanScope();

  v8::Local<v8::Object> instance;
  v8::Local<v8::FunctionTemplate> constructorHandle =
      NanPersistentToLocal(channel_constructor);
  instance = constructorHandle->GetFunction()->NewInstance(0, NULL);
  Channel *c = ObjectWrap::Unwrap<Channel>(instance);
  c->channel = channel;
  c->session = session;
  c->channelClosedCallback = channelClosedCallback;
  c->callbackUserData = callbackUserData;

  return scope.Close(instance);
}

NAN_METHOD(Channel::Start) {
  NanScope();

  Channel *c = ObjectWrap::Unwrap<Channel>(args.This());
  c->SetupCallbacks(false);

  NanReturnUndefined();
}

NAN_METHOD(Channel::New) {
  NanScope();

  Channel* obj = new Channel();
  obj->Wrap(args.This());
  if (NSSH_DEBUG)
    std::cout << "Channel::New()" << std::endl;

  NanReturnValue(args.This());
}

NAN_METHOD(Channel::WriteData) {
  NanScope();

  //TODO: async
  Channel* c = node::ObjectWrap::Unwrap<Channel>(args.This());
  ssh_channel_write(c->channel,
      node::Buffer::Data(args[0]), node::Buffer::Length(args[0]));

  NanReturnUndefined();
}

NAN_METHOD(Channel::SendExitStatus) {
  NanScope();

  //TODO: async
  Channel* c = node::ObjectWrap::Unwrap<Channel>(args.This());
  ssh_channel_request_send_exit_status(c->channel, args[0]->Int32Value());

  NanReturnUndefined();
}

NAN_METHOD(Channel::Close) {
  NanScope();

  //TODO: async
  Channel* c = node::ObjectWrap::Unwrap<Channel>(args.This());
  c->CloseChannel();

  NanReturnUndefined();
}

NAN_METHOD(Channel::SendEof) {
  NanScope();

  //TODO: async
  Channel* c = node::ObjectWrap::Unwrap<Channel>(args.This());
  if (!c->closed)
    ssh_channel_send_eof(c->channel);

  if (NSSH_DEBUG)
    std::cout << "ssh_channel_send_eof()\n";

  NanReturnUndefined();
}

} // namespace nssh
