const stream = require('stream')
    , util   = require('util')

function Channel (server, channel) {
  this._channel = channel
  this._server  = server

  channel.onMessage = function (message) {
    if (this._server._options.debug)
      console.log('channel message', message)
    if (message.subtype && /^(exec|subsystem|shell|pty|env|windowchange)$/.test(message.subtype))
      return this.emit(message.subtype, message)
    // else handle default.. probably should pass this on
    message.replyDefault()
  }.bind(this)

  channel.onSftpMessage = function (message) {
    if (this._server._options.debug)
      console.log('sftp message', message)
    this.emit('sftp:' + message.type, message)
    this.emit('sftpmessage', message)
  }.bind(this)

  channel.onData = this.push.bind(this)

  channel.onClose = this.push.bind(this, null)

  stream.Duplex.call(this)

  process.nextTick(function () {
    this.on('finish', function () {
      this._channel.sendEof()
      //this._channel.close()
    }.bind(this))
  }.bind(this))
}

util.inherits(Channel, stream.Duplex)

Channel.prototype.close = function () {
  this._channel.close()
  return this
}

Channel.prototype._read = function (size) {
  //TODO: implement, there's no backpressure here!
}

Channel.prototype._write = function (chunk, encoding, callback) {
  this._channel.writeData(chunk)
  callback()
}

Channel.prototype.sendEof = function () {
  this._channel.sendEof()
}

Channel.prototype.sendExitStatus = function (code) {
  this._channel.sendExitStatus(Number(code))
}

module.exports = Channel