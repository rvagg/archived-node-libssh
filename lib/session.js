const EventEmitter = require('events').EventEmitter
    , util         = require('util')
    , Channel      = require('./channel')

function Session (session) {
  this._session = session

  session.onMessage = function (message) {
    if (message.type == 'auth')
      return this.emit('auth', message)

    // else handle default.. probably should pass this on
    message.replyDefault()
  }.bind(this)

  session.onNewChannel = function (channel) {
    this.emit('channel', new Channel(channel))
  }.bind(this)

  EventEmitter.call(this)
}

util.inherits(Session, EventEmitter)

Session.prototype.close = function () {
  this._session.close()
  return this
}

module.exports = Session