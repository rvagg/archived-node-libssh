/* Copyright (c) 2013 Rod Vagg
 * MIT +no-false-attribs License <https://github.com/rvagg/node-ssh/blob/master/LICENSE>
 */
#ifndef NSSH_H
#define NSSH_H

#include <node.h>
//#include <node_buffer.h>

#define NSSH_DEBUG true

static inline char* FromV8String(v8::Local<v8::Value> from) {
  size_t sz_;
  char* to;
  v8::Local<v8::String> toStr = from->ToString();
  sz_ = toStr->Utf8Length();
  to = new char[sz_ + 1];
  toStr->WriteUtf8(to, -1, NULL, v8::String::NO_OPTIONS);
  return to;
}

#define NSSH_THROW_RETURN(...)                                                 \
  v8::ThrowException(v8::Exception::Error(v8::String::New(#__VA_ARGS__)));     \
  return v8::Undefined();

#define NSSH_V8_METHOD(name)                                                   \
  static v8::Handle<v8::Value> name (const v8::Arguments& args);

#endif
