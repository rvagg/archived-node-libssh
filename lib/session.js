const EventEmitter = require('events').EventEmitter
    , util         = require('util')
    , Channel      = require('./channel')

function Session (server, session) {
  this._session = session
  this._server  = server

  session.onMessage = function (message) {
    if (this._server._options.debug)
      console.log('session message', message)
    if (message.type == 'auth')
      return this.emit('auth', message)
    // else handle default.. probably should pass this on
    message.replyDefault()
  }.bind(this)

  session.onNewChannel = function (channel) {
    this.emit('channel', new Channel(this._server, channel))
  }.bind(this)

  EventEmitter.call(this)
}

util.inherits(Session, EventEmitter)

Session.prototype.close = function () {
  this._session.close()
  return this
}

module.exports = Session