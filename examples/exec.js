const libssh = require('../')
    , fs     = require('fs')
    , spawn  = require('child_process').spawn

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
    channel.on('exec', function (message) {
      message.replySuccess() // a success reply is needed before we send output

      var child = spawn('/bin/bash', ['-c', message.execCommand])

      child.stdout.pipe(channel)
      channel.pipe(child.stdin)

      child.on('close', function (code) {
        channel.sendExitStatus(code)
        channel.close()
      })
    })
  })
})

server.listen(3333)
console.log('Listening on port 3333')
