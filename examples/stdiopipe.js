const libssh = require('../')
    , fs     = require('fs')

// connect with: ssh -p 3333 localhost -l '$ecretb@ckdoor'
// password 'nsa'
// or: ssh -p 3333 localhost -i ../test/keys/id_rsa

var server = libssh.createServer({
    hostRsaKeyFile : __dirname + '/../test/keys/host_rsa'
  , hostDsaKeyFile : __dirname + '/../test/keys/host_dsa'
})

server.on('connection', function (session) {
  session.on('auth', function (message) {
    if (message.subtype == 'publickey'
        && message.authUser == '$ecretb@ckdoor'
        && message.comparePublicKey(
            fs.readFileSync(__dirname + '/../test/keys/id_rsa.pub'))) {
      // matching keypair, correct user
      return message.replyAuthSuccess()
    }

    if (message.subtype == 'password'
        && message.authUser == '$ecretb@ckdoor'
        && message.authPassword == 'nsa') {
      // correct user, matching password
      return message.replyAuthSuccess()
    }
    message.replyDefault() // auth failed
  })

  session.on('channel', function (channel) {
    channel.on('end', function () {
      // current channel ended
    })
    channel.on('exec', function (message) {
      // execute `message.execCommand`
    })
    channel.on('subsystem', function (message) {
      // `message.subsystem` tells you what's requested
      // could be 'sftp'
    })
    channel.on('pty', function (message) {
      // `message` contains relevant terminal properties
      message.replySuccess()
    })
    channel.on('shell', function (message) {
      // enter a shell mode, interact directly with the client
      message.replySuccess()
      // `channel` is a duplex stream allowing you to interact with
      // the client

      channel.write('Welcome to my party!\n')
      // lets do a console chat via ssh!
      process.stdin                  // take stdin and pipe it to the channel
        .pipe(channel.pipe(channel)) // pipe the channel to itself for an echo
        .pipe(process.stdout)        // pipe the channel to stdout
    })
  })
})

server.listen(3333)
console.log('Listening on port 3333')
