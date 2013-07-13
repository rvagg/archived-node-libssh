/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */
#include <node.h>
#include <iostream>
#include <libssh/server.h>
#include <libssh/keys.h>
#include <libssh/sftp.h>
#include <string.h>
#include "message.h"

namespace nssh {

const char* Message::MessageTypeToString (int type) {
  return type == SSH_REQUEST_AUTH ? "auth"
    : type == SSH_REQUEST_CHANNEL ? "channel"
      : type == SSH_REQUEST_SERVICE ? "service"
        : type == SSH_REQUEST_GLOBAL ? "global"
          : "unknown";
}

const char* Message::MessageSubtypeToString (int type, int subtype) {
  switch (type) {
    case SSH_REQUEST_AUTH:
      switch (subtype) {
        case SSH_AUTH_METHOD_NONE:
          return "none";
        case SSH_AUTH_METHOD_PASSWORD:
          return "password";
        case SSH_AUTH_METHOD_PUBLICKEY:
          return "publickey";
        case SSH_AUTH_METHOD_HOSTBASED:
          return "hostbased";
        case SSH_AUTH_METHOD_INTERACTIVE:
          return "interactive";
        case SSH_AUTH_METHOD_UNKNOWN:
        default:
          return "unknwon";
      }
    case SSH_REQUEST_CHANNEL_OPEN:
      switch (subtype) {
        case SSH_CHANNEL_SESSION:
          return "session";
        case SSH_CHANNEL_DIRECT_TCPIP:
          return "directtcpip";
        case SSH_CHANNEL_FORWARDED_TCPIP:
          return "forwardedtcpip";
        case SSH_CHANNEL_X11:
          return "x11";
        case SSH_CHANNEL_UNKNOWN:
        default:
          return "unknown";
      }
    case SSH_REQUEST_CHANNEL:
      switch (subtype) {
        case SSH_CHANNEL_REQUEST_PTY:
          return "pty";
        case SSH_CHANNEL_REQUEST_EXEC:
          return "exec";
        case SSH_CHANNEL_REQUEST_SHELL:
          return "shell";
        case SSH_CHANNEL_REQUEST_ENV:
          return "env";
        case SSH_CHANNEL_REQUEST_SUBSYSTEM:
          return "subsystem";
        case SSH_CHANNEL_REQUEST_WINDOW_CHANGE:
          return "windowchange";
        case SSH_CHANNEL_REQUEST_X11:
          return "x11";
        case SSH_CHANNEL_REQUEST_UNKNOWN:
        default:
          return "unknown";
      }
    case SSH_REQUEST_GLOBAL:
      switch (subtype) {
        case SSH_GLOBAL_REQUEST_TCPIP_FORWARD:
          return "tcpipforward";
        case SSH_GLOBAL_REQUEST_CANCEL_TCPIP_FORWARD:
          return "canceltcpipforward";
        case SSH_GLOBAL_REQUEST_UNKNOWN:
        default:
          return "unknown";
      }
  }

  return "";
}

v8::Persistent<v8::Function> Message::constructor;
v8::Persistent<v8::String> MessageTypeSymbol;
v8::Persistent<v8::String> MessageSubtypeSymbol;
v8::Persistent<v8::String> MessageAuthUserSymbol;
v8::Persistent<v8::String> MessageAuthPasswordSymbol;
v8::Persistent<v8::String> MessageExecCommandSymbol;
v8::Persistent<v8::String> MessageSubsystemSymbol;
v8::Persistent<v8::String> MessagePtyWidthSymbol;
v8::Persistent<v8::String> MessagePtyHeightSymbol;
v8::Persistent<v8::String> MessagePtyPxWidthSymbol;
v8::Persistent<v8::String> MessagePtyPxHeightSymbol;
v8::Persistent<v8::String> MessagePtyTermSymbol;

Message::Message () {
}

Message::~Message () {
  ssh_message_free(message);
}

void Message::Init () {
  v8::HandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  tpl->SetClassName(v8::String::NewSymbol("Message"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  node::SetPrototypeMethod(tpl, "replyDefault", ReplyDefault);
  node::SetPrototypeMethod(tpl, "replyAuthSuccess", ReplyAuthSuccess);
  node::SetPrototypeMethod(tpl, "replySuccess", ReplySuccess);
  node::SetPrototypeMethod(tpl, "comparePublicKey", ComparePublicKey);
  node::SetPrototypeMethod(tpl, "scpAccept", ScpAccept);
  node::SetPrototypeMethod(tpl, "sftpAccept", SftpAccept);
  constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());

  MessageTypeSymbol = NODE_PSYMBOL("type");
  MessageSubtypeSymbol = NODE_PSYMBOL("subtype");
  MessageAuthUserSymbol = NODE_PSYMBOL("authUser");
  MessageAuthPasswordSymbol = NODE_PSYMBOL("authPassword");
  MessageExecCommandSymbol = NODE_PSYMBOL("execCommand");
  MessageSubsystemSymbol = NODE_PSYMBOL("subsystem");
  MessagePtyWidthSymbol = NODE_PSYMBOL("ptyWidth");
  MessagePtyHeightSymbol = NODE_PSYMBOL("ptyHeight");
  MessagePtyPxWidthSymbol = NODE_PSYMBOL("ptyPxWidth");
  MessagePtyPxHeightSymbol = NODE_PSYMBOL("ptyPxHeight");
  MessagePtyTermSymbol = NODE_PSYMBOL("ptyTerm");
}

v8::Handle<v8::Object> Message::NewInstance (
      ssh_session session
    , Channel *channel
    , ssh_message message) {

  v8::HandleScope scope;

  if (NSSH_DEBUG)
    std::cout << "Message::NewInstance\n";

  v8::Local<v8::Object> instance = constructor->NewInstance(0, NULL);
  Message *m = ObjectWrap::Unwrap<Message>(instance);
  m->session = session;
  m->channel = channel;
  m->message = message;

  if (NSSH_DEBUG)
    std::cout << "Message::NewInstance got instance\n";

  int type = ssh_message_type(message);
  int subtype = ssh_message_subtype(message);
  const char *typeStr = MessageTypeToString(type);
  const char *subtypeStr = MessageSubtypeToString(type, subtype);

  instance->Set(MessageTypeSymbol,
      typeStr == NULL ? v8::Null() : v8::String::New(typeStr));
  instance->Set(MessageSubtypeSymbol,
      subtypeStr == NULL ? v8::Null() : v8::String::New(subtypeStr));

  if (type == SSH_REQUEST_AUTH) {
    const char *authUser = ssh_message_auth_user(message);
    if (authUser)
      instance->Set(MessageAuthUserSymbol, v8::String::New(authUser));
    const char *authPassword = ssh_message_auth_password(message);
    if (authPassword)
      instance->Set(MessageAuthPasswordSymbol, v8::String::New(authPassword));
  } else if (type == SSH_REQUEST_CHANNEL) {
    if (subtype == SSH_CHANNEL_REQUEST_EXEC) {
      const char *execCommand = ssh_message_channel_request_command(message);
      if (execCommand)
        instance->Set(MessageExecCommandSymbol, v8::String::New(execCommand));
    } else if (subtype == SSH_CHANNEL_REQUEST_SUBSYSTEM) {
      const char *subsystem = ssh_message_channel_request_subsystem(message);
      if (subsystem) 
        instance->Set(MessageSubsystemSymbol, v8::String::New(subsystem));
    } else if (subtype == SSH_CHANNEL_REQUEST_PTY) {
      instance->Set(MessagePtyWidthSymbol,
          v8::Integer::New(ssh_message_channel_request_pty_width(message)));
      instance->Set(MessagePtyHeightSymbol,
          v8::Integer::New(ssh_message_channel_request_pty_height(message)));
      instance->Set(MessagePtyPxWidthSymbol,
          v8::Integer::New(ssh_message_channel_request_pty_pxwidth(message)));
      instance->Set(MessagePtyPxHeightSymbol,
          v8::Integer::New(ssh_message_channel_request_pty_pxheight(message)));
      const char *term = ssh_message_channel_request_pty_term(message);
      if (term)
        instance->Set(MessagePtyTermSymbol, v8::String::New(term));
    }
  }

  return scope.Close(instance);
}

v8::Handle<v8::Value> Message::New (const v8::Arguments& args) {
  v8::HandleScope scope;

  Message* obj = new Message();
  obj->Wrap(args.This());

  if (NSSH_DEBUG)
    std::cout << "Message::New()" << std::endl;

  return scope.Close(args.This());
}

v8::Handle<v8::Value> Message::ReplyDefault (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());
  ssh_message_reply_default(m->message);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Message::ReplySuccess (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());
  ssh_message_channel_request_reply_success(m->message);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Message::ReplyAuthSuccess (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());
  ssh_message_auth_reply_success(m->message, 0);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> Message::ComparePublicKey (const v8::Arguments& args) {
  v8::HandleScope scope;

  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());
  ssh_key userkey = ssh_key_new();
  ssh_key key = ssh_message_auth_pubkey(m->message);
  char *keyData = node::Buffer::Data(args[0]);
  //int len = node::Buffer::Length(args[0]);

  char *p = keyData;
  const char *q = keyData;
  while (!isspace((int)*p)) p++;
  *p = '\0';
  enum ssh_keytypes_e type = ssh_key_type_from_name(q);
  if (type == SSH_KEYTYPE_UNKNOWN) {
    if (NSSH_DEBUG)
      std::cout << "ERROR!! UNKNOWN KEYTYPE\n";
    v8::ThrowException(v8::Exception::Error(
        v8::String::New("Error: unknown key type")));
    return scope.Close(v8::Undefined());
  }
  q = ++p;
  while (!isspace((int)*p)) p++;
  *p = '\0';

  int rc = ssh_pki_import_pubkey_base64(q, type, &userkey);
  if (rc) {
    v8::ThrowException(v8::Exception::Error(
        v8::String::New("Error: could not read Base64 key data")));
    return scope.Close(v8::Undefined());
  }

  int cmp = ssh_key_cmp(userkey, key, SSH_KEY_CMP_PUBLIC);
  if (NSSH_DEBUG)
    std::cout << "ssh_key_cmp for " << ssh_message_auth_user(m->message) << ": "
      << cmp << std::endl;
  if (NSSH_DEBUG)
    std::cout << "[" << q << "]\n";
  ssh_key_free(userkey);

  return scope.Close(cmp == 0 ? v8::True() : v8::False());
}

v8::Handle<v8::Value> Message::SftpAccept (const v8::Arguments& args) {
  v8::HandleScope scope;

  if (NSSH_DEBUG)
    std::cout << "Message::SftpAccept\n";

  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());

  sftp_session sftp = sftp_server_new(m->session, m->channel->channel);
  m->channel->SetSftp(sftp);

  return scope.Close(v8::Undefined());
}

// meh, not really working...
v8::Handle<v8::Value> Message::ScpAccept (const v8::Arguments& args) {
  v8::HandleScope scope;

  if (NSSH_DEBUG)
    std::cout << "Message::ScpAccept\n";

  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());

  ssh_scp scp = ssh_scp_new(m->session, SSH_SCP_WRITE, "/tmp/scp/");

  int rc = ssh_scp_init(scp);
  if (!rc) {
    if (NSSH_DEBUG)
      std::cout << "ERROR on SCP INIT!\n";
  }

  rc = ssh_scp_pull_request(scp);
  if (rc != SSH_SCP_REQUEST_NEWFILE) {
    if (NSSH_DEBUG)
      std::cout << "ERROR NOT SSH_SCP_REQUEST_NEWFILE! " << rc << std::endl;
  }

  //TODO: async
  ssh_scp_accept_request(scp);

  if (NSSH_DEBUG)
    std::cout << "ssh_scp_accept_request()... ?\n";

  char buf[1024];
  int len = ssh_scp_read(scp, buf, sizeof(buf));

  if (NSSH_DEBUG)
    std::cout << "ssh_scp_read(): " << len << " = [" << buf << "]\n";

  return scope.Close(v8::Undefined());
}

} // namespace nssh
