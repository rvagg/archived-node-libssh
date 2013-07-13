const libssh = require('../')
    , test   = require('tap').test
    , net    = require('net')

test('test create arguments & options', function (t) {
  t.type(libssh.createServer, 'function', 'createServer is a function')

  t.throws(function () {
    libssh.createServer()
  }, 'requires options')

  t.throws(function () {
    libssh.createServer({})
  }, 'requires options')

  t.end()
})

test('test createServer() starts a server', function (t) {
  t.plan(11)

  var server = libssh.createServer({
      hostRsaKeyFile : __dirname + '/keys/host_rsa'
    , hostDsaKeyFile : __dirname + '/keys/host_dsa'
  })

  t.ok(server, 'got a server!')

  t.type(server.on, 'function', 'server is event listener')
  t.type(server.once, 'function', 'server is event listener')
  t.type(server.removeListener, 'function', 'server is event listener')

  t.throws(server.listen, 'requires a port option')

  server.on('close', function () {
    t.pass('close event called')
  })

  server.on('ready', function () {
    t.pass('ready event called')
  })

  t.equal(server, server.listen(3333, function (err, _server) {
    t.notOk(err, 'no error')
    t.equal(server, _server, 'returns same server object')

    // great! now lets poke it and see if there's something listening

    var socket = new net.Socket()
    socket.on('close', function () {
      server.close(function (err) {
        t.notOk(err, 'no error')
      })
    })
    socket.on('error', function (err) {
      t.fail(err)
    })
    socket.on('connect', function () {
      socket.destroy()
    })
    socket.connect(3333)
  }), 'server.listen() returns self')
})