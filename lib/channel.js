const stream = require('stream')
    , util   = require('util')

function Channel (channel) {
  this._channel = channel

  channel.onMessage = function (message) {
    // else handle default.. probably should pass this on
    if (message.subtype && /^(exec|subsystem|shell|pty)$/.test(message.subtype))
      return this.emit(message.subtype, message)
    message.replyDefault()
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
  console.error('close()')
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

module.exports = Channel