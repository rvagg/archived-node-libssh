/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */
#include <node.h>
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

v8::Persistent<v8::Function> Channel::constructor;
v8::Persistent<v8::String> ChannelOnMessageSymbol;
v8::Persistent<v8::String> ChannelOnSftpMessageSymbol;
v8::Persistent<v8::String> ChannelOnDataSymbol;

Channel::Channel () {
  sftp = NULL;
  callbacks = NULL;
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
  c->CloseChannel();
}

void ChannelCloseCallback (
      ssh_session session
    , ssh_channel channel
    , void *userdata) {

  Channel* c = static_cast<Channel*>(userdata);
  c->CloseChannel();
  if (NSSH_DEBUG)
    std::cout << "ChannelCloseCallback!\n";
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
  if (channelClosedCallback)
    channelClosedCallback(this, callbackUserData);
  if (NSSH_DEBUG)
    std::cout << "ssh_channel_close()\n";
  ssh_channel_close(channel);
  ssh_channel_free(channel);
}

bool Channel::TryRead () {
  sftp_client_message sftpmessage;

  if (sftp) {
    if (NSSH_DEBUG)
      std::cout << "sftp=true\n";
    sftpmessage = sftp_get_client_message(sftp);
    if (sftpmessage) {
      if (NSSH_DEBUG)
        std::cout << "TryRead sftp Message " << sftpmessage << std::endl;
      v8::Handle<v8::Object> mess = SftpMessage::NewInstance(session, this, sftpmessage);
      OnSftpMessage(mess);
      return true;
    }
  }

  char buf[1024];
  int len = ssh_channel_read_nonblocking(channel, buf, sizeof(buf), 0);
  if (len > 0)
    OnData(buf, len);

  if (NSSH_DEBUG)
    std::cout << "Channel::TryRead len=" << len << std::endl;
  return len > 0;
}

bool Channel::IsChannel (ssh_channel channel) {
  return this->channel == channel;
}

void Channel::OnMessage (v8::Handle<v8::Object> mess) {
  v8::HandleScope scope;

  if (NSSH_DEBUG)
    std::cout << "Channel::OnMessage\n";

  v8::Local<v8::Value> callback = this->handle_->Get(ChannelOnMessageSymbol);

  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = { mess };
    callback.As<v8::Function>()->Call(this->handle_, 1, argv);

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Channel::OnSftpMessage (v8::Handle<v8::Object> mess) {
  v8::HandleScope scope;

  if (NSSH_DEBUG)
    std::cout << "Channel::OnSftpMessage\n";

  v8::Local<v8::Value> callback = this->handle_->Get(ChannelOnSftpMessageSymbol);

  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = { mess };
    callback.As<v8::Function>()->Call(this->handle_, 1, argv);

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Channel::OnData (const char *data, int length) {
  v8::HandleScope scope;

  v8::Local<v8::Value> callback = this->handle_->Get(ChannelOnDataSymbol);

  if (callback->IsFunction()) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> argv[] = {
      node::Buffer::New(data, length)->handle_
    };

    callback.As<v8::Function>()->Call(this->handle_, 1, argv);

    if (try_catch.HasCaught())
      node::FatalException(try_catch);
  }
}

void Channel::Init () {
  v8::HandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  tpl->SetClassName(v8::String::NewSymbol("Channel"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  node::SetPrototypeMethod(tpl, "writeData", WriteData);
  node::SetPrototypeMethod(tpl, "sendExitStatus", SendExitStatus);
  node::SetPrototypeMethod(tpl, "close", Close);
  node::SetPrototypeMethod(tpl, "sendEof", SendEof);
  constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());
  ChannelOnDataSymbol = NODE_PSYMBOL("onData");
  ChannelOnMessageSymbol = NODE_PSYMBOL("onMessage");
  ChannelOnSftpMessageSymbol = NODE_PSYMBOL("onSftpMessage");
}

v8::Handle<v8::Object> Channel::NewInstance (
      ssh_session session
    , ssh_channel channel
    , ChannelClosedCallback channelClosedCallback
    , void *callbackUserData
  ) {

  v8::HandleScope scope;

  v8::Local<v8::Object> instance = constructor->NewInstance(0, NULL);
  Channel *s = ObjectWrap::Unwrap<Channel>(instance);
  s->channel = channel;
  s->session = session;
  s->channelClosedCallback = channelClosedCallback;
  s->callbackUserData = callbackUserData;

  s->SetupCallbacks(true);

  return scope.Close(instance);
}

v8::Handle<v8::Value> Channel::New (const v8::Arguments& args) {
  v8::HandleScope scope;

  Channel* obj = new Channel();
  obj->Wrap(args.This());
  if (NSSH_DEBUG)
    std::cout << "Channel::New()" << std::endl;

  return scope.Close(args.This());
}

v8::Handle<v8::Value> Channel::WriteData (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  Channel* c = node::ObjectWrap::Unwrap<Channel>(args.This());
  ssh_channel_write(c->channel,
      node::Buffer::Data(args[0]), node::Buffer::Length(args[0]));

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Channel::SendExitStatus (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  Channel* c = node::ObjectWrap::Unwrap<Channel>(args.This());
  ssh_channel_request_send_exit_status(c->channel, args[0]->Int32Value());

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Channel::Close (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  Channel* c = node::ObjectWrap::Unwrap<Channel>(args.This());
  c->CloseChannel();

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Channel::SendEof (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  Channel* c = node::ObjectWrap::Unwrap<Channel>(args.This());
  ssh_channel_send_eof(c->channel);

  if (NSSH_DEBUG)
    std::cout << "ssh_channel_send_eof()\n";

  return scope.Close(v8::Undefined());
}

} // namespace nssh
