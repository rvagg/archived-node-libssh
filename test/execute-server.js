const libssh = require('../')
    , SSH2   = require('ssh2')

function executeServerTest (t, connectOptions, authCb, channelCb, connectionCb) {
  var server = libssh.createServer({
      hostRsaKeyFile : __dirname + '/keys/host_rsa'
    , hostDsaKeyFile : __dirname + '/keys/host_dsa'
  })

  server.on('connection', function (session) {
    t.ok(session, '(execute-server) have a session object!')
    session.on('auth', function (message) {
      t.ok(session, '(execute-server) have a message object, triggered "auth" event')
      authCb(message)
    })
    session.on('channel', function (channel) {
      t.ok(channel, '(execute-server) have a channel!')
      channelCb(channel, session)
      channel.on('end', function () {
        t.pass('(execute-server) got "end" event')
      })

      /*
      channel.on('exec', function (message) { })
      channel.on('subsystem', function (message) { })
      channel.on('pty', function (message) { })
      channel.on('shell', function (message) { })
      */
    })
  })

  server.listen(3333, function () {
    var connection = new SSH2()
    connection.connect(connectOptions)
    connection.on('ready', function () {
      connectionCb(connection)
    })
    connection.on('connect', function () { })
    connection.on('error', function (err) {
      console.error('connection error')
      t.fail(err)
    })
    connection.on('end', function () {
    })
    connection.on('close', function () {
      server.close()
      setTimeout(function () {
        t.pass('(execute-server) closing')
        t.end()
      }, 100)
    })
  })

  return server
}
executeServerTest.plan = 5

module.exports = executeServerTest