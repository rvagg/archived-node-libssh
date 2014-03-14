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

v8::Persistent<v8::FunctionTemplate> message_constructor;

Message::Message () {
}

Message::~Message () {
  //TODO: GC is knocking this off too quick, either need to figure out
  // if libssh is doing this for us or protect Message from GC until
  // it's no longer needed -- what's the trigger for this?
  //ssh_message_free(message);
}

void Message::Init () {
  NanScope();

  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  NanAssignPersistent(v8::FunctionTemplate, message_constructor, tpl);
  tpl->SetClassName(NanSymbol("Message"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  NODE_SET_PROTOTYPE_METHOD(tpl, "replyDefault", ReplyDefault);
  NODE_SET_PROTOTYPE_METHOD(tpl, "replyAuthSuccess", ReplyAuthSuccess);
  NODE_SET_PROTOTYPE_METHOD(tpl, "replySuccess", ReplySuccess);
  NODE_SET_PROTOTYPE_METHOD(tpl, "comparePublicKey", ComparePublicKey);
  NODE_SET_PROTOTYPE_METHOD(tpl, "scpAccept", ScpAccept);
  NODE_SET_PROTOTYPE_METHOD(tpl, "sftpAccept", SftpAccept);
}

v8::Handle<v8::Object> Message::NewInstance (
      ssh_session session
    , Channel *channel
    , ssh_message message) {

  NanScope();

  if (NSSH_DEBUG)
    std::cout << "Message::NewInstance\n";

  v8::Local<v8::FunctionTemplate> constructorHandle =
      NanPersistentToLocal(message_constructor);
  v8::Local<v8::Object> instance =
      constructorHandle->GetFunction()->NewInstance(0, NULL);

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

  instance->Set(NanSymbol("type"), typeStr == NULL
    ? v8::Null().As<v8::Object>()
    : v8::String::New(typeStr).As<v8::Object>()
  );
  instance->Set(NanSymbol("subtype"), subtypeStr == NULL
    ? v8::Null().As<v8::Object>()
    : v8::String::New(subtypeStr).As<v8::Object>()
  );

  if (type == SSH_REQUEST_AUTH) {
    const char *authUser = ssh_message_auth_user(message);
    if (authUser)
      instance->Set(NanSymbol("authUser"), v8::String::New(authUser));
    const char *authPassword = ssh_message_auth_password(message);
    if (authPassword)
      instance->Set(NanSymbol("authPassword"), v8::String::New(authPassword));
  } else if (type == SSH_REQUEST_CHANNEL) {
    if (subtype == SSH_CHANNEL_REQUEST_EXEC) {
      const char *execCommand = ssh_message_channel_request_command(message);
      if (execCommand)
        instance->Set(NanSymbol("execCommand"), v8::String::New(execCommand));
    } else if (subtype == SSH_CHANNEL_REQUEST_SUBSYSTEM) {
      const char *subsystem = ssh_message_channel_request_subsystem(message);
      if (subsystem) 
        instance->Set(NanSymbol("subsystem"), v8::String::New(subsystem));
    } else if (subtype == SSH_CHANNEL_REQUEST_PTY) {
      instance->Set(NanSymbol("ptyWidth"),
          v8::Integer::New(ssh_message_channel_request_pty_width(message)));
      instance->Set(NanSymbol("ptyHeight"),
          v8::Integer::New(ssh_message_channel_request_pty_height(message)));
      instance->Set(NanSymbol("ptyPxWidth"),
          v8::Integer::New(ssh_message_channel_request_pty_pxwidth(message)));
      instance->Set(NanSymbol("ptyPxHeight"),
          v8::Integer::New(ssh_message_channel_request_pty_pxheight(message)));
      const char *term = ssh_message_channel_request_pty_term(message);
      if (term)
        instance->Set(NanSymbol("ptyTerm"), v8::String::New(term));
    } else if (subtype == SSH_CHANNEL_REQUEST_WINDOW_CHANGE) {
      instance->Set(NanSymbol("ptyWidth"),
          v8::Integer::New(ssh_message_channel_request_pty_width(message)));
      instance->Set(NanSymbol("ptyHeight"),
          v8::Integer::New(ssh_message_channel_request_pty_height(message)));
    }
  }

  return scope.Close(instance);
}

NAN_METHOD(Message::New) {
  NanScope();

  Message* obj = new Message();
  obj->Wrap(args.This());

  if (NSSH_DEBUG)
    std::cout << "Message::New()" << std::endl;

  NanReturnValue(args.This());
}

NAN_METHOD(Message::ReplyDefault) {
  NanScope();

  //TODO: async
  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());
  ssh_message_reply_default(m->message);

  NanReturnUndefined();
}

NAN_METHOD(Message::ReplySuccess) {
  NanScope();

  //TODO: async
  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());
  ssh_message_channel_request_reply_success(m->message);

  NanReturnUndefined();
}

NAN_METHOD(Message::ReplyAuthSuccess) {
  NanScope();

  //TODO: async
  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());
  ssh_message_auth_reply_success(m->message, 0);

  NanReturnUndefined();
}

NAN_METHOD(Message::ComparePublicKey) {
  NanScope();

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
    return NanThrowError("Error: unknown key type");
  }
  q = ++p;
  while (!isspace((int)*p)) p++;
  *p = '\0';

  int rc = ssh_pki_import_pubkey_base64(q, type, &userkey);
  if (rc) {
    return NanThrowError("Error: could not read Base64 key data");
  }

  int cmp = ssh_key_cmp(userkey, key, SSH_KEY_CMP_PUBLIC);
  if (NSSH_DEBUG)
    std::cout << "ssh_key_cmp for " << ssh_message_auth_user(m->message) << ": "
      << cmp << std::endl;
  if (NSSH_DEBUG)
    std::cout << "[" << q << "]\n";
  ssh_key_free(userkey);

  NanReturnValue(cmp == 0 ? v8::True() : v8::False());
}

NAN_METHOD(Message::SftpAccept) {
  NanScope();

  if (NSSH_DEBUG)
    std::cout << "Message::SftpAccept\n";

  Message* m = node::ObjectWrap::Unwrap<Message>(args.This());

  sftp_session sftp = sftp_server_new(m->session, m->channel->channel);
  m->channel->SetSftp(sftp);

  NanReturnUndefined();
}

// meh, not really working...
NAN_METHOD(Message::ScpAccept) {
  NanScope();

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

  NanReturnUndefined();
}

} // namespace nssh
