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

  if (str->Equals(NanNew<v8::String>("ok"))) {
    return SSH_FX_OK;
  } else if (str->Equals(NanNew<v8::String>("eof"))) {
    return SSH_FX_EOF;
  } else if (str->Equals(NanNew<v8::String>("noSuchFile"))) {
    return SSH_FX_NO_SUCH_FILE;
  } else if (str->Equals(NanNew<v8::String>("permissionDenied"))) {
    return SSH_FX_PERMISSION_DENIED;
  } else if (str->Equals(NanNew<v8::String>("failure"))) {
    return SSH_FX_FAILURE;
  } else if (str->Equals(NanNew<v8::String>("badMessage"))) {
    return SSH_FX_BAD_MESSAGE;
  } else if (str->Equals(NanNew<v8::String>("noConnection"))) {
    return SSH_FX_NO_CONNECTION;
  } else if (str->Equals(NanNew<v8::String>("connectionLost"))) {
    return SSH_FX_CONNECTION_LOST;
  } else if (str->Equals(NanNew<v8::String>("opUnsupported"))) {
    return SSH_FX_OP_UNSUPPORTED;
  } else if (str->Equals(NanNew<v8::String>("invalidHandle"))) {
    return SSH_FX_INVALID_HANDLE;
  } else if (str->Equals(NanNew<v8::String>("noSuchPath"))) {
    return SSH_FX_NO_SUCH_PATH;
  } else if (str->Equals(NanNew<v8::String>("fileAlreadyExists"))) {
    return SSH_FX_FILE_ALREADY_EXISTS;
  } else if (str->Equals(NanNew<v8::String>("writeProtect"))) {
    return SSH_FX_WRITE_PROTECT;
  } else if (str->Equals(NanNew<v8::String>("noMedia"))) {
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

  v8::Local<v8::FunctionTemplate> tpl = NanNew<v8::FunctionTemplate>(New);
  NanAssignPersistent(sftpmessage_constructor, tpl);
  tpl->SetClassName(NanNew<v8::String>("SftpMessage"));
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

  NanEscapableScope();

  if (NSSH_DEBUG)
    std::cout << "SftpMessage::NewInstance\n";

  v8::Local<v8::FunctionTemplate> constructorHandle = NanNew(sftpmessage_constructor);
  v8::Local<v8::Object> instance =
      constructorHandle->GetFunction()->NewInstance(0, NULL);

  SftpMessage *m = ObjectWrap::Unwrap<SftpMessage>(instance);
  m->session = session;
  m->channel = channel;
  m->message = message;

  if (NSSH_DEBUG)
    std::cout << "SftpMessage::NewInstance got instance\n";

  const char *typeStr = MessageTypeToString(message->type);
  instance->Set(NanNew<v8::String>("type"), typeStr == NULL
    ? NanNull()->ToObject()
    : NanNew<v8::String>(typeStr)->ToObject()
  );

  switch(message->type) {
    case SSH_FXP_CLOSE:
    case SSH_FXP_READDIR:
      if (NSSH_DEBUG)
        std::cout << "SSH_FXP_READDIR: "
          << (const char *)message->handle->data << ":"
          << ssh_string_len(message->handle) << std::endl;
      instance->Set(NanNew<v8::String>("handle"), NanNew<v8::String>(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      break;
    case SSH_FXP_READ:
      instance->Set(NanNew<v8::String>("handle"), NanNew<v8::String>(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(NanNew<v8::String>("offset"), NanNew<v8::Integer>(message->offset));
      instance->Set(NanNew<v8::String>("length"), NanNew<v8::Integer>(message->len));
      break;
    case SSH_FXP_WRITE:
      instance->Set(NanNew<v8::String>("handle"), NanNew<v8::String>(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      if (NSSH_DEBUG)
        std::cout << "read `data`, " << ssh_string_len(message->data)
          << " bytes\n";
      instance->Set(NanNew<v8::String>("offset"), NanNew<v8::Integer>(message->offset));
      instance->Set(NanNew<v8::String>("data"), NanNewBufferHandle(
          (char *)ssh_string_get_char(message->data)
        , ssh_string_len(message->data)
      ));
      break;
    case SSH_FXP_REMOVE:
    case SSH_FXP_RMDIR:
    case SSH_FXP_OPENDIR:
    case SSH_FXP_READLINK:
    case SSH_FXP_REALPATH:
      instance->Set(NanNew<v8::String>("filename"), NanNew<v8::String>(
          (const char *)message->filename));
      break;
    case SSH_FXP_RENAME:
    case SSH_FXP_SYMLINK:
      instance->Set(NanNew<v8::String>("filename"), NanNew<v8::String>(
          (const char *)message->filename));
      instance->Set(NanNew<v8::String>("data"), NanNew<v8::String>(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      break;
    case SSH_FXP_MKDIR:
    case SSH_FXP_SETSTAT:
      instance->Set(NanNew<v8::String>("filename"), NanNew<v8::String>(
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
      instance->Set(NanNew<v8::String>("handle"), NanNew<v8::String>(
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
      instance->Set(NanNew<v8::String>("filename"), NanNew<v8::String>(
          (const char *)message->filename));
      instance->Set(NanNew<v8::String>("flags"), NanNew<v8::Integer>(message->flags));
      /* TODO: flags, array of strings? rwa?
      if(sftp->version > 3) {
        buffer_get_u32(payload,&msg->flags);
      }
      */
      break;
    case SSH_FXP_OPEN:
      instance->Set(NanNew<v8::String>("filename"), NanNew<v8::String>(
          (const char *)message->filename));
      if (NSSH_DEBUG)
        std::cout << "SSH_FXP_OPEN flags = " << message->flags << std::endl;
      instance->Set(NanNew<v8::String>("handle"), NanNew<v8::String>(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(NanNew<v8::String>("flags"), NanNew<v8::Integer>(message->flags));
      //TODO: use node core style mode: http://nodejs.org/docs/latest/api/fs.html#fs_fs_open_path_flags_mode_callback
      /* TODO: flags, array of strings? rwa?
#define SSH_FXF_READ 0x01
#define SSH_FXF_WRITE 0x02
#define SSH_FXF_APPEND 0x04
#define SSH_FXF_CREAT 0x08
#define SSH_FXF_TRUNC 0x10
#define SSH_FXF_EXCL 0x20
#define SSH_FXF_TEXT 0x40
      instance->Set(NanNew<v8::String>("flags"), NanNew<v8::String>(
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
      instance->Set(NanNew<v8::String>("handle"), NanNew<v8::String>(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(NanNew<v8::String>("flags"), NanNew<v8::Integer>(message->flags));
      break;
  }

  return NanEscapeScope(instance);
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

  v8::String::Utf8Value prop(object->Get(property));
  return *prop;
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

  if (HasStringProperty(attrObject, NanNew<v8::String>("filename"))) {
    attr->name = GetStringProperty(attrObject, NanNew<v8::String>("filename")); //TODO: free
    attr->longname = attr->name;
  }

  if (HasStringProperty(attrObject, NanNew<v8::String>("type"))) {
    v8::Local<v8::String> type =
        attrObject->Get(NanNew<v8::String>("type")).As<v8::String>();
    attr->type = 0;
    if (type->Equals(NanNew<v8::String>("regular")))
      attr->type = SSH_FILEXFER_TYPE_REGULAR;
    else if (type->Equals(NanNew<v8::String>("directory")))
      attr->type = SSH_FILEXFER_TYPE_DIRECTORY;
    else if (type->Equals(NanNew<v8::String>("symlink")))
      attr->type = SSH_FILEXFER_TYPE_SYMLINK;
    else if (type->Equals(NanNew<v8::String>("special")))
      attr->type = SSH_FILEXFER_TYPE_SPECIAL;
    else if (type->Equals(NanNew<v8::String>("unknown")))
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

  if (HasIntegerProperty(attrObject, NanNew<v8::String>("size"))) {
    attr->size = GetIntegerProperty(attrObject, NanNew<v8::String>("size"));
    attr->flags |= SSH_FILEXFER_ATTR_SIZE;
  }

  bool hasUid = HasIntegerProperty(attrObject, NanNew<v8::String>("uid"));
  bool hasGid = HasIntegerProperty(attrObject, NanNew<v8::String>("gid"));
  if (hasUid || hasGid) {
    attr->flags |= SSH_FILEXFER_ATTR_UIDGID;
    if (hasUid)
      attr->uid = GetIntegerProperty(attrObject, NanNew<v8::String>("uid"));
    else
      attr->uid = 0;
    if (hasGid)
      attr->gid = GetIntegerProperty(attrObject, NanNew<v8::String>("gid"));
    else
      attr->gid = 0;
  }

  if (HasStringProperty(attrObject, NanNew<v8::String>("owner"))) {
    attr->owner = GetStringProperty(attrObject, NanNew<v8::String>("owner")); //TODO: free
    attr->flags |= SSH_FILEXFER_ATTR_OWNERGROUP;
  }

  if (HasStringProperty(attrObject, NanNew<v8::String>("group"))) {
    attr->group = GetStringProperty(attrObject, NanNew<v8::String>("group")); //TODO: free
    attr->flags |= SSH_FILEXFER_ATTR_OWNERGROUP;
  }

  bool hasCtime = HasIntegerProperty(attrObject, NanNew<v8::String>("ctime"));
  bool hasMtime = HasIntegerProperty(attrObject, NanNew<v8::String>("mtime"));
  bool hasAtime = HasIntegerProperty(attrObject, NanNew<v8::String>("atime"));
  if (hasCtime || hasMtime || hasAtime) {
    attr->flags |= SSH_FILEXFER_ATTR_ACMODTIME;
    if (hasCtime) {
      attr->createtime = GetIntegerProperty(attrObject, NanNew<v8::String>("ctime"));
      attr->flags |= SSH_FILEXFER_ATTR_CREATETIME;
    } else
      attr->createtime = 0;

    if (hasMtime) {
      attr->mtime64 = GetIntegerProperty(attrObject, NanNew<v8::String>("mtime"));
      attr->mtime = attr->mtime64;
      attr->flags |= SSH_FILEXFER_ATTR_MODIFYTIME;
    } else {
      attr->mtime = 0;
      attr->mtime64 = 0;
    }

    if (HasIntegerProperty(attrObject, NanNew<v8::String>("atime"))) {
      attr->atime64 = GetIntegerProperty(attrObject, NanNew<v8::String>("atime"));
      attr->atime = attr->atime64;
      attr->flags |= SSH_FILEXFER_ATTR_ACCESSTIME;
    } else {
      attr->atime = 0;
      attr->atime64 = 0;
    }
  }

  if (HasIntegerProperty(attrObject, NanNew<v8::String>("permissions"))) {
    attr->permissions = GetIntegerProperty(attrObject, NanNew<v8::String>("permissions"));
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

  v8::String::Utf8Value s(args[0]);

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  sftp_reply_name(
      m->message
    , *s
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

    v8::String::Utf8Value fname(obj->Get(NanNew<v8::String>("filename")));
    v8::String::Utf8Value lname(obj->Get(NanNew<v8::String>("longname")));

    if (NSSH_DEBUG)
      std::cout << "Name [" << *fname << "] [" << *lname << "]\n";

    sftp_reply_names_add(
        m->message
      , *fname //TODO: free (I think?)
      , *lname //TODO: free (I think?)
      , obj->Has(NanNew<v8::String>("attrs"))
          ? ObjectToAttributes(obj->Get(NanNew<v8::String>("attrs")).As<v8::Object>())
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

  v8::String::Utf8Value s(args[0]);

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  sftp_reply_handle(m->message, ssh_string_from_char(*s)); // free x 2

  NanReturnUndefined();
}

NAN_METHOD(SftpMessage::ReplyStatus) {
  NanScope();

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  if (NSSH_DEBUG)
    std::cout << "ReplyStatus: " << StringToStatusCode(args[0].As<v8::String>())
      << std::endl;

  uint32_t status_code = StringToStatusCode(args[0].As<v8::String>());
  if (args.Length() > 1) {
    v8::String::Utf8Value s(args[1]);
    sftp_reply_status(m->message, status_code, *s);
  } else {
    sftp_reply_status(m->message, status_code, NULL);
  }

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
