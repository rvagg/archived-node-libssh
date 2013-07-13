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

v8::Persistent<v8::Function> SftpMessage::constructor;
v8::Persistent<v8::String> SftpMessageTypeSymbol;
v8::Persistent<v8::String> SftpHandleSymbol;
v8::Persistent<v8::String> SftpFlagsSymbol;
v8::Persistent<v8::String> SftpOffsetSymbol;
v8::Persistent<v8::String> SftpLengthSymbol;
v8::Persistent<v8::String> SftpDataSymbol;
v8::Persistent<v8::String> SftpFilenameSymbol;
v8::Persistent<v8::String> SftpLongnameSymbol;
v8::Persistent<v8::String> SftpAttributesSymbol;
v8::Persistent<v8::String> SftpTypeSymbol;
v8::Persistent<v8::String> SftpRegularSymbol;
v8::Persistent<v8::String> SftpDirectorySymbol;
v8::Persistent<v8::String> SftpSymlinkSymbol;
v8::Persistent<v8::String> SftpSpecialSymbol;
v8::Persistent<v8::String> SftpUnknownSymbol;
/*
v8::Persistent<v8::String> SftpSocketSymbol;
v8::Persistent<v8::String> SftpCharDeviceSymbol;
v8::Persistent<v8::String> SftpBlockDeviceSymbol;
v8::Persistent<v8::String> SftpFifoSymbol;
*/
v8::Persistent<v8::String> SftpSizeSymbol;
v8::Persistent<v8::String> SftpUidSymbol;
v8::Persistent<v8::String> SftpGidSymbol;
v8::Persistent<v8::String> SftpOwnerSymbol;
v8::Persistent<v8::String> SftpGroupSymbol;
v8::Persistent<v8::String> SftpAtimeSymbol;
v8::Persistent<v8::String> SftpCtimeSymbol;
v8::Persistent<v8::String> SftpMtimeSymbol;
v8::Persistent<v8::String> SftpPermissionsSymbol;

v8::Persistent<v8::String> SftpOkSymbol;
v8::Persistent<v8::String> SftpEofSymbol;
v8::Persistent<v8::String> SftpNoSuchFileSymbol;
v8::Persistent<v8::String> SftpPermissionDeniedSymbol;
v8::Persistent<v8::String> SftpFailureSymbol;
v8::Persistent<v8::String> SftpBadMessageSymbol;
v8::Persistent<v8::String> SftpNoConnectionSymbol;
v8::Persistent<v8::String> SftpConnectionLostSymbol;
v8::Persistent<v8::String> SftpOpUnsupportedSymbol;
v8::Persistent<v8::String> SftpInvalidHandleSymbol;
v8::Persistent<v8::String> SftpNoSuchPathSymbol;
v8::Persistent<v8::String> SftpFileAlreadyExistsSymbol;
v8::Persistent<v8::String> SftpWriteProtectSymbol;
v8::Persistent<v8::String> SftpNoMediaSymbol;
/*
v8::Persistent<v8::String> SftpNoSpaceOnFilesystemSymbol;
v8::Persistent<v8::String> SftpQuotaExceededSymbol;
v8::Persistent<v8::String> SftpUnknownPrincipalSymbol;
v8::Persistent<v8::String> SftpLockConflictSymbol;
v8::Persistent<v8::String> SftpDirNotEmptySymbol;
v8::Persistent<v8::String> SftpNotADirectorySymbol;
v8::Persistent<v8::String> SftpInvalidFilenameSymbol;
v8::Persistent<v8::String> SftpLinkLoopSymbol;
v8::Persistent<v8::String> SftpCannotDeleteSymbol;
v8::Persistent<v8::String> SftpInvalidParameterSymbol;
v8::Persistent<v8::String> SftpFileIsADirectorySymbol;
v8::Persistent<v8::String> SftpByteRangeLockConflictSymbol;
v8::Persistent<v8::String> SftpByteRangeLockRefusedSymbol;
v8::Persistent<v8::String> SftpDeletePendingSymbol;
v8::Persistent<v8::String> SftpFileCorruptSymbol;
v8::Persistent<v8::String> SftpOwnerInvalidSymbol;
v8::Persistent<v8::String> SftpGroupInvalidSymbol;
*/

inline uint32_t StringToStatusCode (v8::Handle<v8::String> str) {
  if (str->Equals(SftpOkSymbol)) {
    return SSH_FX_OK;
  } else if (str->Equals(SftpEofSymbol)) {
    return SSH_FX_EOF;
  } else if (str->Equals(SftpNoSuchFileSymbol)) {
    return SSH_FX_NO_SUCH_FILE;
  } else if (str->Equals(SftpPermissionDeniedSymbol)) {
    return SSH_FX_PERMISSION_DENIED;
  } else if (str->Equals(SftpFailureSymbol)) {
    return SSH_FX_FAILURE;
  } else if (str->Equals(SftpBadMessageSymbol)) {
    return SSH_FX_BAD_MESSAGE;
  } else if (str->Equals(SftpNoConnectionSymbol)) {
    return SSH_FX_NO_CONNECTION;
  } else if (str->Equals(SftpConnectionLostSymbol)) {
    return SSH_FX_CONNECTION_LOST;
  } else if (str->Equals(SftpOpUnsupportedSymbol)) {
    return SSH_FX_OP_UNSUPPORTED;
  } else if (str->Equals(SftpInvalidHandleSymbol)) {
    return SSH_FX_INVALID_HANDLE;
  } else if (str->Equals(SftpNoSuchPathSymbol)) {
    return SSH_FX_NO_SUCH_PATH;
  } else if (str->Equals(SftpFileAlreadyExistsSymbol)) {
    return SSH_FX_FILE_ALREADY_EXISTS;
  } else if (str->Equals(SftpWriteProtectSymbol)) {
    return SSH_FX_WRITE_PROTECT;
  } else if (str->Equals(SftpNoMediaSymbol)) {
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
  v8::HandleScope scope;
  v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
  tpl->SetClassName(v8::String::NewSymbol("SftpMessage"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  node::SetPrototypeMethod(tpl, "replyName", ReplyName);
  node::SetPrototypeMethod(tpl, "replyNames", ReplyNames);
  node::SetPrototypeMethod(tpl, "replyAttr", ReplyAttr);
  node::SetPrototypeMethod(tpl, "replyHandle", ReplyHandle);
  node::SetPrototypeMethod(tpl, "replyStatus", ReplyStatus);
  node::SetPrototypeMethod(tpl, "replyData", ReplyData);
  constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());

  SftpMessageTypeSymbol = NODE_PSYMBOL("type");
  SftpHandleSymbol = NODE_PSYMBOL("handle");
  SftpFlagsSymbol = NODE_PSYMBOL("flags");
  SftpOffsetSymbol = NODE_PSYMBOL("offset");
  SftpLengthSymbol = NODE_PSYMBOL("length");
  SftpDataSymbol = NODE_PSYMBOL("data");
  SftpFilenameSymbol = NODE_PSYMBOL("filename");
  SftpLongnameSymbol = NODE_PSYMBOL("longname");
  SftpAttributesSymbol = NODE_PSYMBOL("attrs");
  SftpTypeSymbol = NODE_PSYMBOL("type");
  SftpRegularSymbol = NODE_PSYMBOL("regular");
  SftpDirectorySymbol = NODE_PSYMBOL("directory");
  SftpSymlinkSymbol = NODE_PSYMBOL("symlink");
  SftpSpecialSymbol = NODE_PSYMBOL("special");
  SftpUnknownSymbol = NODE_PSYMBOL("unknown");
  /*
  SftpSocketSymbol = NODE_PSYMBOL("socket");
  SftpCharDeviceSymbol = NODE_PSYMBOL("char_device");
  SftpBlockDeviceSymbol = NODE_PSYMBOL("block_device");
  SftpFifoSymbol = NODE_PSYMBOL("fifo");
  */
  SftpSizeSymbol = NODE_PSYMBOL("size");
  SftpUidSymbol = NODE_PSYMBOL("uid");
  SftpGidSymbol = NODE_PSYMBOL("gid");
  SftpOwnerSymbol = NODE_PSYMBOL("owner");
  SftpGroupSymbol = NODE_PSYMBOL("group");
  SftpAtimeSymbol = NODE_PSYMBOL("atime");
  SftpCtimeSymbol = NODE_PSYMBOL("ctime");
  SftpMtimeSymbol = NODE_PSYMBOL("mtime");
  SftpPermissionsSymbol = NODE_PSYMBOL("permissions");

  SftpOkSymbol = NODE_PSYMBOL("ok");
  SftpEofSymbol = NODE_PSYMBOL("eof");
  SftpNoSuchFileSymbol = NODE_PSYMBOL("noSuchFile");
  SftpPermissionDeniedSymbol = NODE_PSYMBOL("permissionDenied");
  SftpFailureSymbol = NODE_PSYMBOL("failure");
  SftpBadMessageSymbol = NODE_PSYMBOL("badMessage");
  SftpNoConnectionSymbol = NODE_PSYMBOL("noConnection");
  SftpConnectionLostSymbol = NODE_PSYMBOL("connectionLost");
  SftpOpUnsupportedSymbol = NODE_PSYMBOL("opUnsupported");
  SftpInvalidHandleSymbol = NODE_PSYMBOL("invalidHandle");
  SftpNoSuchPathSymbol = NODE_PSYMBOL("noSuchPath");
  SftpFileAlreadyExistsSymbol = NODE_PSYMBOL("fileAlreadyExists");
  SftpWriteProtectSymbol = NODE_PSYMBOL("writeProtect");
  SftpNoMediaSymbol = NODE_PSYMBOL("noMedia");
/*
  SftpNoSpaceOnFilesystemSymbol = NODE_PSYMBOL("noSpaceOnFilesystem");
  SftpQuotaExceededSymbol = NODE_PSYMBOL("quotaExceeded");
  SftpUnknownPrincipalSymbol = NODE_PSYMBOL("unknownPrincipal");
  SftpLockConflictSymbol = NODE_PSYMBOL("lockConflict");
  SftpDirNotEmptySymbol = NODE_PSYMBOL("dirNotEmpty");
  SftpNotADirectorySymbol = NODE_PSYMBOL("notADirectory");
  SftpInvalidFilenameSymbol = NODE_PSYMBOL("invalidFilename");
  SftpLinkLoopSymbol = NODE_PSYMBOL("linkLoop");
  SftpCannotDeleteSymbol = NODE_PSYMBOL("cannotDelete");
  SftpInvalidParameterSymbol = NODE_PSYMBOL("invalidParameter");
  SftpFileIsADirectorySymbol = NODE_PSYMBOL("fileIsADirectory");
  SftpByteRangeLockConflictSymbol = NODE_PSYMBOL("byteRangeLockConflict");
  SftpByteRangeLockRefusedSymbol = NODE_PSYMBOL("byteRangeLockRefused");
  SftpDeletePendingSymbol = NODE_PSYMBOL("deletePending");
  SftpFileCorruptSymbol = NODE_PSYMBOL("fileCorrupt");
  SftpOwnerInvalidSymbol = NODE_PSYMBOL("ownerInvalid");
  SftpGroupInvalidSymbol = NODE_PSYMBOL("groupInvalid");
*/
}

v8::Handle<v8::Object> SftpMessage::NewInstance (
      ssh_session session
    , Channel *channel
    , sftp_client_message message) {

  v8::HandleScope scope;

  if (NSSH_DEBUG)
    std::cout << "SftpMessage::NewInstance\n";

  v8::Local<v8::Object> instance = constructor->NewInstance(0, NULL);
  SftpMessage *m = ObjectWrap::Unwrap<SftpMessage>(instance);
  m->session = session;
  m->channel = channel;
  m->message = message;

  if (NSSH_DEBUG)
    std::cout << "SftpMessage::NewInstance got instance\n";

  const char *typeStr = MessageTypeToString(message->type);
  instance->Set(SftpMessageTypeSymbol,
      typeStr == NULL ? v8::Null() : v8::String::New(typeStr));

  switch(message->type) {
    case SSH_FXP_CLOSE:
    case SSH_FXP_READDIR:
      if (NSSH_DEBUG)
        std::cout << "SSH_FXP_READDIR: "
          << (const char *)message->handle->data << ":"
          << ssh_string_len(message->handle) << std::endl;
      instance->Set(SftpHandleSymbol, v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      break;
    case SSH_FXP_READ:
      instance->Set(SftpHandleSymbol, v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(SftpOffsetSymbol, v8::Integer::New(message->offset));
      instance->Set(SftpLengthSymbol, v8::Integer::New(message->len));
      break;
    case SSH_FXP_WRITE:
      instance->Set(SftpHandleSymbol, v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(SftpOffsetSymbol, v8::Integer::New(message->offset));
      instance->Set(SftpDataSymbol, node::Buffer::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))->handle_
      );
      break;
    case SSH_FXP_REMOVE:
    case SSH_FXP_RMDIR:
    case SSH_FXP_OPENDIR:
    case SSH_FXP_READLINK:
    case SSH_FXP_REALPATH:
      instance->Set(SftpFilenameSymbol, v8::String::New(
          (const char *)message->filename));
      break;
    case SSH_FXP_RENAME:
    case SSH_FXP_SYMLINK:
      instance->Set(SftpFilenameSymbol, v8::String::New(
          (const char *)message->filename));
      instance->Set(SftpDataSymbol, v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      break;
    case SSH_FXP_MKDIR:
    case SSH_FXP_SETSTAT:
      instance->Set(SftpFilenameSymbol, v8::String::New(
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
      instance->Set(SftpHandleSymbol, v8::String::New(
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
      instance->Set(SftpFilenameSymbol, v8::String::New(
          (const char *)message->filename));
      instance->Set(SftpFlagsSymbol, v8::Integer::New(message->flags));
      /* TODO: flags, array of strings? rwa?
      if(sftp->version > 3) {
        buffer_get_u32(payload,&msg->flags);
      }
      */
      break;
    case SSH_FXP_OPEN:
      instance->Set(SftpFilenameSymbol, v8::String::New(
          (const char *)message->filename));
      if (NSSH_DEBUG)
        std::cout << "SSH_FXP_OPEN flags = " << message->flags << std::endl;
      instance->Set(SftpHandleSymbol, v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(SftpFlagsSymbol, v8::Integer::New(message->flags));
      //TODO: use node core style mode: http://nodejs.org/docs/latest/api/fs.html#fs_fs_open_path_flags_mode_callback
      /* TODO: flags, array of strings? rwa?
#define SSH_FXF_READ 0x01
#define SSH_FXF_WRITE 0x02
#define SSH_FXF_APPEND 0x04
#define SSH_FXF_CREAT 0x08
#define SSH_FXF_TRUNC 0x10
#define SSH_FXF_EXCL 0x20
#define SSH_FXF_TEXT 0x40
      instance->Set(SftpFlagsSymbol, v8::String::New(
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
      instance->Set(SftpHandleSymbol, v8::String::New(
          (const char *)message->handle->data
        , ssh_string_len(message->handle))
      );
      instance->Set(SftpFlagsSymbol, v8::Integer::New(message->flags));
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
  return FromV8String(object->Get(property));
}

static inline bool HasIntegerProperty(
      v8::Handle<v8::Object> object
    , v8::Handle<v8::String> property) {
  return object->Has(property) && object->Get(property)->IsNumber();
}

static inline uint64_t GetIntegerProperty(
      v8::Handle<v8::Object> object
    , v8::Handle<v8::String> property) {
  return object->Get(property)->IntegerValue();
}

sftp_attributes ObjectToAttributes (v8::Local<v8::Object> attrObject) {
  sftp_attributes attr = new sftp_attributes_struct;
  attr->flags = 0;

  if (HasStringProperty(attrObject, SftpFilenameSymbol)) {
    attr->name = GetStringProperty(attrObject, SftpFilenameSymbol); //TODO: free
    attr->longname = attr->name;
  }

  if (HasStringProperty(attrObject, SftpTypeSymbol)) {
    v8::Local<v8::String> type =
        attrObject->Get(SftpTypeSymbol).As<v8::String>();
    attr->type = 0;
    if (type->Equals(SftpRegularSymbol))
      attr->type = SSH_FILEXFER_TYPE_REGULAR;
    else if (type->Equals(SftpDirectorySymbol))
      attr->type = SSH_FILEXFER_TYPE_DIRECTORY;
    else if (type->Equals(SftpSymlinkSymbol))
      attr->type = SSH_FILEXFER_TYPE_SYMLINK;
    else if (type->Equals(SftpSpecialSymbol))
      attr->type = SSH_FILEXFER_TYPE_SPECIAL;
    else if (type->Equals(SftpUnknownSymbol))
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

  if (HasIntegerProperty(attrObject, SftpSizeSymbol)) {
    attr->size = GetIntegerProperty(attrObject, SftpSizeSymbol);
    attr->flags |= SSH_FILEXFER_ATTR_SIZE;
  }

  bool hasUid = HasIntegerProperty(attrObject, SftpUidSymbol);
  bool hasGid = HasIntegerProperty(attrObject, SftpGidSymbol);
  if (hasUid || hasGid) {
    attr->flags |= SSH_FILEXFER_ATTR_UIDGID;
    if (hasUid)
      attr->uid = GetIntegerProperty(attrObject, SftpUidSymbol);
    else
      attr->uid = 0;
    if (hasGid)
      attr->gid = GetIntegerProperty(attrObject, SftpGidSymbol);
    else
      attr->gid = 0;
  }

  if (HasStringProperty(attrObject, SftpOwnerSymbol)) {
    attr->owner = GetStringProperty(attrObject, SftpOwnerSymbol); //TODO: free
    attr->flags |= SSH_FILEXFER_ATTR_OWNERGROUP;
  }

  if (HasStringProperty(attrObject, SftpGroupSymbol)) {
    attr->group = GetStringProperty(attrObject, SftpGroupSymbol); //TODO: free
    attr->flags |= SSH_FILEXFER_ATTR_OWNERGROUP;
  }

  bool hasCtime = HasIntegerProperty(attrObject, SftpCtimeSymbol);
  bool hasMtime = HasIntegerProperty(attrObject, SftpMtimeSymbol);
  bool hasAtime = HasIntegerProperty(attrObject, SftpAtimeSymbol);
  if (hasCtime || hasMtime || hasAtime) {
    attr->flags |= SSH_FILEXFER_ATTR_ACMODTIME;
    if (hasCtime) {
      attr->createtime = GetIntegerProperty(attrObject, SftpCtimeSymbol);
      attr->flags |= SSH_FILEXFER_ATTR_CREATETIME;
    } else
      attr->createtime = 0;

    if (hasMtime) {
      attr->mtime64 = GetIntegerProperty(attrObject, SftpMtimeSymbol);
      attr->mtime = attr->mtime64;
      attr->flags |= SSH_FILEXFER_ATTR_MODIFYTIME;
    } else {
      attr->mtime = 0;
      attr->mtime64 = 0;
    }

    if (HasIntegerProperty(attrObject, SftpAtimeSymbol)) {
      attr->atime64 = GetIntegerProperty(attrObject, SftpAtimeSymbol);
      attr->atime = attr->atime64;
      attr->flags |= SSH_FILEXFER_ATTR_ACCESSTIME;
    } else {
      attr->atime = 0;
      attr->atime64 = 0;
    }
  }

  if (HasIntegerProperty(attrObject, SftpPermissionsSymbol)) {
    attr->permissions = GetIntegerProperty(attrObject, SftpPermissionsSymbol);
    attr->flags |= SSH_FILEXFER_ATTR_PERMISSIONS;
  }

  return attr;
}

v8::Handle<v8::Value> SftpMessage::New (const v8::Arguments& args) {
  v8::HandleScope scope;

  SftpMessage* obj = new SftpMessage();
  obj->Wrap(args.This());
  if (NSSH_DEBUG)
    std::cout << "SftpMessage::New()" << std::endl;

  return scope.Close(args.This());
}

v8::Handle<v8::Value> SftpMessage::ReplyName (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  sftp_reply_name(m->message, FromV8String(args[0]),
      args.Length() > 1 ? ObjectToAttributes(args[1]->ToObject()) : 0);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> SftpMessage::ReplyNames (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(args[0]);
  for (unsigned int i = 0; i < array->Length(); i++) {
    if (!array->Get(i)->IsObject())
      continue;

    v8::Local<v8::Object> obj = array->Get(i).As<v8::Object>();

    if (NSSH_DEBUG)
      std::cout << "Name [" << FromV8String(obj->Get(SftpFilenameSymbol))
        << "] [" <<  FromV8String(obj->Get(SftpLongnameSymbol)) << "]\n";

    sftp_reply_names_add(
        m->message
      , FromV8String(obj->Get(SftpFilenameSymbol)) // free
      , FromV8String(obj->Get(SftpLongnameSymbol)) // free
      , obj->Has(SftpAttributesSymbol)
          ? ObjectToAttributes(obj->Get(SftpAttributesSymbol).As<v8::Object>())
          : 0
    );
  }

  sftp_reply_names(m->message);
  //sftp_reply_status(m->message, SSH_FX_EOF, NULL);

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> SftpMessage::ReplyAttr (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  sftp_reply_attr(m->message, ObjectToAttributes(args[0]->ToObject()));

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> SftpMessage::ReplyHandle (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  sftp_reply_handle(m->message, ssh_string_from_char(FromV8String(args[0]))); // free x 2

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> SftpMessage::ReplyStatus (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  if (NSSH_DEBUG)
    std::cout << "ReplyStatus: " << StringToStatusCode(args[0].As<v8::String>())
      << std::endl;
  sftp_reply_status(
      m->message
    , StringToStatusCode(args[0].As<v8::String>())
    , args.Length() > 1 ? FromV8String(args[1]) : NULL // free
  );

  return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> SftpMessage::ReplyData (const v8::Arguments& args) {
  v8::HandleScope scope;

  //TODO: async
  SftpMessage* m = node::ObjectWrap::Unwrap<SftpMessage>(args.This());
  v8::Local<v8::Object> obj = args[0].As<v8::Object>();
  sftp_reply_data(m->message, node::Buffer::Data(obj), args[1]->IntegerValue());

  return scope.Close(v8::Undefined());
}

} // namespace nssh
