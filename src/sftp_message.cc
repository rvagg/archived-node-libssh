/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */
#include <node.h>
#include <iostream>
#include <libssh/server.h>
#include <libssh/keys.h>
#include <libssh/sftp.h>
#include <libssh/string.h>
#include <string.h>
#include "sftp_message.h"

namespace nssh {

v8::Persistent<v8::FunctionTemplate> sftpmessage_constructor;

inline uint32_t StringToStatusCode (v8::Handle<v8::String> str) {
  NanScope();

  if (str->Equals(NanSymbol("ok"))) {
    return SSH_FX_OK;
  } else if (str->Equals(NanSymbol("eof"))) {
    return SSH_FX_EOF;
  } else if (str->Equals(NanSymbol("noSuchFile"))) {
    return SSH_FX_NO_SUCH_FILE;
  } else if (str->Equals(NanSymbol("permissionDenied"))) {
    return SSH_FX_PERMISSION_DENIED;
  } else if (str->Equals(NanSymbol("failure"))) {
    return SSH_FX_FAILURE;
  } else if (str->Equals(NanSymbol("badMessage"))) {
    return SSH_FX_BAD_MESSAGE;
  } else if (str->Equals(NanSymbol("noConnection"))) {
    return SSH_FX_NO_CONNECTION;
  } else if (str->Equals(NanSymbol("connectionLost"))) {
    return SSH_FX_CONNECTION_LOST;
  } else if (str->Equals(NanSymbol("opUnsupported"))) {
    return SSH_FX_OP_UNSUPPORTED;
  } else if (str->Equals(NanSymbol("invalidHandle"))) {
    return SSH_FX_INVALID_HANDLE;
  } else if (str->Equals(NanSymbol("noSuchPath"))) {
    return SSH_FX_NO_SUCH_PATH;
  } else if (str->Equals(NanSymbol("fileAlreadyExists"))) {
    return SSH_FX_FILE_ALREADY_EXISTS;
  } else if (str->Equals(NanSymbol("writeProtect"))) {
    return SSH_FX_WRITE_PROTECT;
  } else if (str->Equals(NanSymbol("noMedia"))) {
    return SSH_FX_NO_MEDIA;
/*
  } else if (str->Equals(SftpNoSpaceOnFilesystemSymbol)) {
    return SSH_FX_NO_SPACE_ON_FILESYSTEM;
  } else if (str->Equals(SftpQuotaExceededSymbol)) {
    return SSH_FX_QUOTA_EXCEEDED;
  } else if (str->Equals(SftpUnknownPrincipalSymbol)) {
    return SSH_FX_UNKNOWN_PRINCIPAL;
  } else if (str->Equals(SftpLockConflictSymbol)) {
    return SSH_FX_LOCK_CONFLICT;
  } else if (str->Equals(SftpDirNotEmptySymbol)) {
    return SSH_FX_DIR_NOT_EMPTY;
  } else if (str->Equals(SftpNotADirectorySymbol)) {
    return SSH_FX_NOT_A_DIRECTORY;
  } else if (str->Equals(SftpInvalidFilenameSymbol)) {
    return SSH_FX_INVALID_HANDLE;
  } else if (str->Equals(SftpLinkLoopSymbol)) {
    return SSH_FX_LINK_LOOP;
  } else if (str->Equals(SftpCannotDeleteSymbol)) {
    return SSH_FX_CANNOT_DELETE;
  } else if (str->Equals(SftpInvalidParameterSymbol)) {
    return SSH_FX_INVALID_PARAMETER;
  } else if (str->Equals(SftpFileIsADirectorySymbol)) {
    return SSH_FX_FILE_IS_A_DIRECTORY;
  } else if (str->Equals(SftpByteRangeLockConflictSymbol)) {
    return SSH_FX_BYTE_RANGE_LOCK_CONFLICT;
  } else if (str->Equals(SftpByteRangeLockRefusedSymbol)) {
    return SSH_FX_BYTE_RANGE_LOCK_REFUSED;
  } else if (str->Equals(SftpDeletePendingSymbol)) {
    return SSH_FX_DELETE_PENDING;
  } else if (str->Equals(SftpFileCorruptSymbol)) {
    return SSH_FX_FILE_CORRUPT;
  } else if (str->Equals(SftpOwnerInvalidSymbol)) {
    return SSH_FX_OWNER_INVALID;
  } else if (str->Equals(SftpGroupInvalidSymbol)) {
    return SSH_FX_GROUP_INVALID;
*/
  }

  return SSH_FX_OK;
}

inline const char* SftpMessage::MessageTypeToString (int type) {
  switch (type) {
    case SSH_FXP_INIT:
      return "init";
    case SSH_FXP_VERSION:
      return "version";
    case SSH_FXP_OPEN:
      return "open";
    case SSH_FXP_CLOSE:
      return "close";
    case SSH_FXP_READ:
      return "read";
    case SSH_FXP_WRITE:
      return "write";
    case SSH_FXP_LSTAT:
      return "lstat";
    case SSH_FXP_FSTAT:
      return "fstat";
    case SSH_FXP_SETSTAT:
      return "setstat";
    case SSH_FXP_FSETSTAT:
      return "fsetstat";
    case SSH_FXP_OPENDIR:
      return "opendir";
    case SSH_FXP_READDIR:
      return "readdir";
    case SSH_FXP_REMOVE:
      return "remove";
    case SSH_FXP_MKDIR:
      return "mkdir";
    case SSH_FXP_RMDIR:
      return "rmdir";
    case SSH_FXP_REALPATH:
      return "realpath";
    case SSH_FXP_STAT:
      return "stat";
    case SSH_FXP_RENAME:
      return "rename";
    case SSH_FXP_READLINK:
      return "readlink";
    case SSH_FXP_SYMLINK:
      return "symlink";
    default:
      return "unknown";
  }
}

SftpMessage::SftpMessage () {
}

SftpMessage::~SftpMessage () {
  sftp_client_message_free(message);
}

void SftpMessage::Init () {
  NanScope();

  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  NanAssignPersistent(v8::FunctionTemplate, sftpmessage_constructor, tpl);
  tpl->SetClassName(NanSymbol("SftpMessage"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  NODE_SET_PROTOTYPE_METHOD(tpl, "replyName", ReplyName);
  NODE_SET_PROTOTYPE_METHOD(tpl, "replyNames", ReplyNames);
  NODE_SET_PROTOTYPE_METHOD(tpl, "replyAttr", ReplyAttr);
  NODE_SET_PROTOTYPE_METHOD(tpl, "replyHandle", ReplyHandle);
  NODE_SET_PROTOTYPE_METHOD(tpl, "replyStatus", ReplyStatus);
  NODE_SET_PROTOTYPE_METHOD(tpl, "replyData", ReplyData);
}

v8::Handle<v8::Object> SftpMessage::NewInstance (
      ssh_session session
    , Channel *channel
    , sftp_client_message message) {

  NanScope();

  if (NSSH_DEBUG)
    std::cout << "SftpMessage::NewInstance\n";

  v8::Local<v8::FunctionTemplate> constructorHandle =
      NanPersistentToLocal(sftpmessage_constructor);
  v8::Local<v8::Object> instance =
      constructorHandle->GetFunction()->NewInstance(0, NULL);

  SftpMessage *m = ObjectWrap::Unwrap<SftpMessage>(instance);
  m->session = session;
  m->channel = channel;
  m->message = message;

  if (NSSH_DEBUG)
    std::cout << "SftpMessage::NewInstance got instance\n";

  const char *typeStr = MessageTypeToString(message->type);
  instance->Set(NanSymbol("type"), typeStr == NULL
    ? v8::Null().As<v8::Object>()
    : v8::String::New(typeStr).As<v8::Object>()
  );

  switch(message->type) {
    case SSH_FXP_CLOSE:
    case SSH_FXP_READDIR:
      if (NSSH_DEBUG)
        std::cout << "SSH_FXP_READDIR: "
          << (const char *)message->handle->data << ":"
          << ssh_string_len(message->handle) << std::endl;
      instance->Set(NanSymbol("handle"), v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      break;
    case SSH_FXP_READ:
      instance->Set(NanSymbol("handle"), v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(NanSymbol("offset"), v8::Integer::New(message->offset));
      instance->Set(NanSymbol("length"), v8::Integer::New(message->len));
      break;
    case SSH_FXP_WRITE:
      instance->Set(NanSymbol("handle"), v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      if (NSSH_DEBUG)
        std::cout << "read `data`, " << ssh_string_len(message->data)
          << " bytes\n";
      instance->Set(NanSymbol("offset"), v8::Integer::New(message->offset));
      instance->Set(NanSymbol("data"), NanNewBufferHandle(
          (char *)ssh_string_get_char(message->data)
        , ssh_string_len(message->data)
      ));
      break;
    case SSH_FXP_REMOVE:
    case SSH_FXP_RMDIR:
    case SSH_FXP_OPENDIR:
    case SSH_FXP_READLINK:
    case SSH_FXP_REALPATH:
      instance->Set(NanSymbol("filename"), v8::String::New(
          (const char *)message->filename));
      break;
    case SSH_FXP_RENAME:
    case SSH_FXP_SYMLINK:
      instance->Set(NanSymbol("filename"), v8::String::New(
          (const char *)message->filename));
      instance->Set(NanSymbol("data"), v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      break;
    case SSH_FXP_MKDIR:
    case SSH_FXP_SETSTAT:
      instance->Set(NanSymbol("filename"), v8::String::New(
          (const char *)message->filename));
      /*TODO: attr struct..
      msg->attr = sftp_parse_attr(sftp, payload, 0);
      if (msg->attr == NULL) {
        ssh_set_error_oom(session);
        sftp_client_message_free(msg);
        return NULL;
      }
      */
      break;
    case SSH_FXP_FSETSTAT:
      instance->Set(NanSymbol("handle"), v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      /*TODO: attr struct..
      msg->attr = sftp_parse_attr(sftp, payload, 0);
      if (msg->attr == NULL) {
        ssh_set_error_oom(session);
        sftp_client_message_free(msg);
        return NULL;
      }
      */
      break;
    case SSH_FXP_LSTAT:
    case SSH_FXP_STAT:
      instance->Set(NanSymbol("filename"), v8::String::New(
          (const char *)message->filename));
      instance->Set(NanSymbol("flags"), v8::Integer::New(message->flags));
      /* TODO: flags, array of strings? rwa?
      if(sftp->version > 3) {
        buffer_get_u32(payload,&msg->flags);
      }
      */
      break;
    case SSH_FXP_OPEN:
      instance->Set(NanSymbol("filename"), v8::String::New(
          (const char *)message->filename));
      if (NSSH_DEBUG)
        std::cout << "SSH_FXP_OPEN flags = " << message->flags << std::endl;
      instance->Set(NanSymbol("handle"), v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(NanSymbol("flags"), v8::Integer::New(message->flags));
      //TODO: use node core style mode: http://nodejs.org/docs/latest/api/fs.html#fs_fs_open_path_flags_mode_callback
      /* TODO: flags, array of strings? rwa?
#define SSH_FXF_READ 0x01
#define SSH_FXF_WRITE 0x02
#define SSH_FXF_APPEND 0x04
#define SSH_FXF_CREAT 0x08
#define SSH_FXF_TRUNC 0x10
#define SSH_FXF_EXCL 0x20
#define SSH_FXF_TEXT 0x40
      instance->Set(NanSymbol("flags"), v8::String::New(
          (const char *)message->flags));

      buffer_get_u32(payload,&msg->flags);
      */
      /*TODO: attr struct..
      msg->attr = sftp_parse_attr(sftp, payload, 0);
      if (msg->attr == NULL) {
        ssh_set_error_oom(session);
        sftp_client_message_free(msg);
        return NULL;
      }
      */
      break;
    case SSH_FXP_FSTAT:
      instance->Set(NanSymbol("handle"), v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(NanSymbol("flags"), v8::Integer::New(message->flags));
      break;
  }

  return scope.Close(instance);
}

static inline bool HasStringProperty(
      v8::Handle<v8::Object> object
    , v8::Handle<v8::String> property) {
  return object->Has(property) && object->Get(property)->IsString();
}

static inline char* GetStringProperty(
      v8::Handle<v8::Object> object
    , v8::Handle<v8::String> property) {

  NanScope();

  return NanFromV8String(
      object->Get(property).As<v8::Object>()
    , Nan::UTF8
    , NULL
    , NULL
    , 0
    , v8::String::NO_OPTIONS
  );
}

static inline bool HasIntegerProperty(
      v8::Handle<v8::Object> object
    , v8::Handle<v8::String> property) {
  return object->Has(property) && object->Get(property)->IsNumber();
}

static inline uint64_t GetIntegerProperty(
      v8::Handle<v8::Object> object
    , v8::Handle<v8::String> property) {

  NanScope();
  return object->Get(property)->IntegerValue();
}

sftp_attributes ObjectToAttributes (v8::Local<v8::Object> attrObject) {
  NanScope();

  sftp_attributes attr = new sftp_attributes_struct;
  attr->flags = 0;

  if (HasStringProperty(attrObject, NanSymbol("filename"))) {
    attr->name = GetStringProperty(attrObject, NanSymbol("filename")); //TODO: free
    attr->longname = attr->name;
  }

  if (HasStringProperty(attrObject, NanSymbol("type"))) {
    v8::Local<v8::String> type =
        attrObject->Get(NanSymbol("type")).As<v8::String>();
    attr->type = 0;
    if (type->Equals(NanSymbol("regular")))
      attr->type = SSH_FILEXFER_TYPE_REGULAR;
    else if (type->Equals(NanSymbol("directory")))
      attr->type = SSH_FILEXFER_TYPE_DIRECTORY;
    else if (type->Equals(NanSymbol("symlink")))
      attr->type = SSH_FILEXFER_TYPE_SYMLINK;
    else if (type->Equals(NanSymbol("special")))
      attr->type = SSH_FILEXFER_TYPE_SPECIAL;
    else if (type->Equals(NanSymbol("unknown")))
      attr->type = SSH_FILEXFER_TYPE_UNKNOWN;
    /*
    if (type->Equals(SftpSocketSymbol))
      attr->type = SSH_FILEXFER_TYPE_SOCKET;
    if (type->Equals(SftpCharDeviceSymbol))
      attr->type = SSH_FILEXFER_TYPE_CHAR_DEVICE;
    if (type->Equals(SftpBlockDeviceSymbol))
      attr->type = SSH_FILEXFER_TYPE_BLOCK_DEVICE;
    if (type->Equals(SftpFifoSymbol))
      attr->type = SSH_FILEXFER_TYPE_FIFO;
    */
    if (NSSH_DEBUG)
      std::cout << "attr->type = " << (uint32_t)attr->type << std::endl;
  }

  if (HasIntegerProperty(attrObject, NanSymbol("size"))) {
    attr->size = GetIntegerProperty(attrObject, NanSymbol("size"));
    attr->flags |= SSH_FILEXFER_ATTR_SIZE;
  }

  bool hasUid = HasIntegerProperty(attrObject, NanSymbol("uid"));
  bool hasGid = HasIntegerProperty(attrObject, NanSymbol("gid"));
  if (hasUid || hasGid) {
    attr->flags |= SSH_FILEXFER_ATTR_UIDGID;
    if (hasUid)
      attr->uid = GetIntegerProperty(attrObject, NanSymbol("uid"));
    else
      attr->uid = 0;
    if (hasGid)
      attr->gid = GetIntegerProperty(attrObject, NanSymbol("gid"));
    else
      attr->gid = 0;
  }

  if (HasStringProperty(attrObject, NanSymbol("owner"))) {
    attr->owner = GetStringProperty(attrObject, NanSymbol("owner")); //TODO: free
    attr->flags |= SSH_FILEXFER_ATTR_OWNERGROUP;
  }

  if (HasStringProperty(attrObject, NanSymbol("group"))) {
    attr->group = GetStringProperty(attrObject, NanSymbol("group")); //TODO: free
    attr->flags |= SSH_FILEXFER_ATTR_OWNERGROUP;
  }

  bool hasCtime = HasIntegerProperty(attrObject, NanSymbol("ctime"));
  bool hasMtime = HasIntegerProperty(attrObject, NanSymbol("mtime"));
  bool hasAtime = HasIntegerProperty(attrObject, NanSymbol("atime"));
  if (hasCtime || hasMtime || hasAtime) {
    attr->flags |= SSH_FILEXFER_ATTR_ACMODTIME;
    if (hasCtime) {
      attr->createtime = GetIntegerProperty(attrObject, NanSymbol("ctime"));
      attr->flags |= SSH_FILEXFER_ATTR_CREATETIME;
    } else
      attr->createtime = 0;

    if (hasMtime) {
      attr->mtime64 = GetIntegerProperty(attrObject, NanSymbol("mtime"));
      attr->mtime = attr->mtime64;
      attr->flags |= SSH_FILEXFER_ATTR_MODIFYTIME;
    } else {
      attr->mtime = 0;
      attr->mtime64 = 0;
    }

    if (HasIntegerProperty(attrObject, NanSymbol("atime"))) {
      attr->atime64 = GetIntegerProperty(attrObject, NanSymbol("atime"));
      attr->atime = attr->atime64;
      attr->flags |= SSH_FILEXFER_ATTR_ACCESSTIME;
    } else {
      attr->atime = 0;
      attr->atime64 = 0;
    }
  }

  if (HasIntegerProperty(attrObject, NanSymbol("permissions"))) {
    attr->permissions = GetIntegerProperty(attrObject, NanSymbol("permissions"));
    attr->flags |= SSH_FILEXFER_ATTR_PERMISSIONS;
  }

  return attr;
}

NAN_METHOD(SftpMessage::New) {
  NanScope();

  SftpMessage* obj = new SftpMessage();
  obj->Wrap(args.This());
  if (NSSH_DEBUG)
    std::cout << "SftpMessage::New()" << std::endl;

  NanReturnValue(args.This());
}

NAN_METHOD(SftpMessage::ReplyName) {
  NanScope();

  char *s = NanFromV8String(
      args[0].As<v8::Object>()
    , Nan::UTF8
    , NULL
    , NULL
    , 0
    , v8::String::NO_OPTIONS
  );

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  sftp_reply_name(
      m->message
    , s
    , args.Length() > 1 ? ObjectToAttributes(args[1]->ToObject()) : 0
  );

  NanReturnUndefined();
}

NAN_METHOD(SftpMessage::ReplyNames) {
  NanScope();

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(args[0]);
  for (unsigned int i = 0; i < array->Length(); i++) {
    if (!array->Get(i)->IsObject())
      continue;

    v8::Local<v8::Object> obj = array->Get(i).As<v8::Object>();

    char *fname = NanFromV8String(
        obj->Get(NanSymbol("filename")).As<v8::Object>()
      , Nan::UTF8
      , NULL
      , NULL
      , 0
      , v8::String::NO_OPTIONS
    );
    char *lname = NanFromV8String(
        obj->Get(NanSymbol("longname")).As<v8::Object>()
      , Nan::UTF8
      , NULL
      , NULL
      , 0
      , v8::String::NO_OPTIONS
    );

    if (NSSH_DEBUG)
      std::cout << "Name [" << fname << "] [" << lname << "]\n";

    sftp_reply_names_add(
        m->message
      , fname //TODO: free (I think?)
      , lname //TODO: free (I think?)
      , obj->Has(NanSymbol("attrs"))
          ? ObjectToAttributes(obj->Get(NanSymbol("attrs")).As<v8::Object>())
          : 0
    );
  }

  sftp_reply_names(m->message);
  //sftp_reply_status(m->message, SSH_FX_EOF, NULL);

  NanReturnUndefined();
}

NAN_METHOD(SftpMessage::ReplyAttr) {
  NanScope();

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  sftp_reply_attr(m->message, ObjectToAttributes(args[0]->ToObject()));

  NanReturnUndefined();
}

NAN_METHOD(SftpMessage::ReplyHandle) {
  NanScope();

  char *s = NanFromV8String(
      args[0].As<v8::Object>()
    , Nan::UTF8
    , NULL
    , NULL
    , 0
    , v8::String::NO_OPTIONS
  );

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  sftp_reply_handle(m->message, ssh_string_from_char(s)); // free x 2

  NanReturnUndefined();
}

NAN_METHOD(SftpMessage::ReplyStatus) {
  NanScope();

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  if (NSSH_DEBUG)
    std::cout << "ReplyStatus: " << StringToStatusCode(args[0].As<v8::String>())
      << std::endl;

  sftp_reply_status(
      m->message
    , StringToStatusCode(args[0].As<v8::String>())
    , args.Length() > 1
        ? NanFromV8String(
              args[1].As<v8::Object>()
            , Nan::UTF8
            , NULL
            , NULL
            , 0
            , v8::String::NO_OPTIONS
          ) // free
        : NULL
  );

  NanReturnUndefined();
}

NAN_METHOD(SftpMessage::ReplyData) {
  NanScope();

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  v8::Local<v8::Object> obj = args[0].As<v8::Object>();
  sftp_reply_data(m->message, node::Buffer::Data(obj), args[1]->IntegerValue());

  NanReturnUndefined();
}

} // namespace nssh
