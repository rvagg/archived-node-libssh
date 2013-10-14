const libssh = require('../')
    , fs     = require('fs')
    , pty    = require('pty.js')

// connect with ssh -p 3333 localhost -i ../test/keys/id_rsa <cmd>
// where '<cmd>' is a command that can be executed on the server

var server = libssh.createServer({
    hostRsaKeyFile : __dirname + '/../test/keys/host_rsa'
  , hostDsaKeyFile : __dirname + '/../test/keys/host_dsa'
})

server.on('connection', function (session) {
  session.on('auth', function (message) {
    if (message.subtype == 'publickey'
        && message.comparePublicKey(
            fs.readFileSync(__dirname + '/../test/keys/id_rsa.pub'))) {
      // could check message.authUser if we cared about the username
      return message.replyAuthSuccess()
    }
    message.replyDefault() // auth failed
  })

  session.on('channel', function (channel) {
    channel.on('pty', function(message) {
      message.replySuccess()
    })

    channel.on('shell', function(message) {
      message.replySuccess()

      var term = pty.spawn('bash', [], {
        name: 'xterm-color',
        cols: 80,
        rows: 30,
        cwd: process.env.HOME,
        env: process.env
      });

      channel.pipe(term)
      term.pipe(channel)

      term.on('exit', function (code) {
        channel.sendExitStatus(code)
        channel.close()
      })
    })
  })
})

server.listen(3333)
console.log('Listening on port 3333')
