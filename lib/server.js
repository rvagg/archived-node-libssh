const libssh       = require('bindings')('ssh.node')
    , EventEmitter = require('events').EventEmitter
    , util         = require('util')
    , Session      = require('./session')

function Server (options) {
  if (!(this instanceof Server))
    return new Server(options)

  if (typeof options != 'object')
    throw new Error('createServer() requires an options object')
  if (!options.hostRsaKeyFile)
    throw new Error('createServer() requires a `hostRsaKeyFile` option')
  if (!options.hostDsaKeyFile)
    throw new Error('createServer() requires a `hostDsaKeyFile` option')
  if (!options.banner)
    options.banner = "node-libssh"

  this._options = options

  EventEmitter.call(this)
}

util.inherits(Server, EventEmitter)

function setupServer (server) {
  server._server.onConnection = function (session) {
    server.emit('connection', new Session(server, session))
  }
}

Server.prototype.listen = function (port, callback) {
  if (!port)
    throw new Error('listen() requires a `port` argument')

  process.nextTick(function () {
    this._server = new libssh.Server(
        port
      , this._options.hostRsaKeyFile
      , this._options.hostDsaKeyFile
      , this._options.banner
    )
    setupServer(this)
    this.emit('ready')
    if (callback)
      callback(null, this)
  }.bind(this))

  return this
}

Server.prototype.close = function (callback) {
  process.nextTick(function () {
    this._server.close()
    this.emit('close')
    if (callback)
      callback(null, this)
  }.bind(this))
};

module.exports = Server
