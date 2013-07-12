const libssh = require('bindings')('ssh.node')

function Server (options) {
  if (!(this instanceof Server))
    return new Server(options)

}

module.exports = Server