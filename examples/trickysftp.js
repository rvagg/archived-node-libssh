const libssh = require('../')
    , fs     = require('fs')

// connect with: sftp -P 3333 localhost
// do an `ls`, then `get X` one of the files and it should be the same
// as our 'fileforyou.txt' that we serve up for every request

var server = libssh.createServer({
    hostRsaKeyFile : __dirname + '/../test/keys/host_rsa'
  , hostDsaKeyFile : __dirname + '/../test/keys/host_dsa'
})

server.on('connection', function (session) {
  session.on('auth', function (message) {
    // we're just going to let everyone in to this party!
    return message.replyAuthSuccess()
  })

  // authenticated sessions can open channels, you need to react to
  // events for each channel
  session.on('channel', function (channel) {
    channel.on('subsystem', function (message) {
      if (message.subsystem == 'sftp') {
        // we have to indicate success and also accept a switch to
        // SFTP mode
        message.replySuccess()
        message.sftpAccept()
      }
    })

    // after switching in to sftp mode with `message.sftpAccept()` we will
    // now be receiving 'sftp:X' messages, where *X* is an SFTP command
    // the messages are also emitted on the 'sftpmessage' event, for convenience

    channel.on('sftp:realpath', function (message) {
      // client wants to know the real path to the given file/directory
      // provided in `message.filename`, we respond with a `message.replyName()`
      // we don't have to be truthful...
      if (message.filename == '.' || (/\/$/).test(message.filename)) {
        message.replyName('/foo/bar/', {
           permissions: +libssh.Stat('755').dir() // see below for info about Stat
        })
      } else {
        message.replyName('fileforyou.txt', {
           permissions: +libssh.Stat('644').reg()
        })
      }
    })

    channel.on('sftp:stat', statHandle)

    function statHandle (message) {
      // `message.filename` contains the path the client wants to stat

      // let's play a game of "pretend":
      var attrs = {
          permissions: +libssh.Stat(644).reg()
        , uid: 101
        , gid: 202
        , size: fs.statSync('fileforyou.txt').size // must be accurate
        , atime: Date.now()
        , mtime: Date.now()
      }

      message.replyAttr(attrs)
    }

    // can be handled the same way as 'stat' if you like
    channel.on('sftp:lstat', statHandle)

    channel.on('sftp:opendir', function (message) {
      // client wants to move to a given directory, you must return a 'handle'
      // that represents that directory, it can just be the directory name
      message.replyHandle(message.filename)
    })

    // see sftp:readdir to know why we're doing this
    var lastmsg
    channel.on('sftpmessage', function (message) {
      lastmsg = message
    })

    channel.on('sftp:readdir', function (message) {
      // client wants to read the directory, you must respond using
      // `message.replyNames()` with an array of files and their attributes

      // the client will keep sending a 'readdir' until you give it an
      // OK, so you have to keep track of state. 'sftpmessage' is emitted after
      // sftp:* so you can easily use it for this purpose

      // you probably should be more intelligent than this which will return
      // the same list for each readdir:

      if (lastmsg.type == 'readdir')
        return message.replyStatus('ok')

      message.replyNames([
          { filename: 'foo', longname: 'foo'
              , attrs: { permissions: +libssh.Stat(644).reg() } }
        , { filename: 'bar', longname: 'bar'
              , attrs: { permissions: +libssh.Stat(750).dir() } }
        , { filename: 'baz', longname: 'baz'
              , attrs: { permissions: +libssh.Stat(600).reg() } }
      ])
    })

    var openHandles = {}

    channel.on('sftp:open', function (message) {
      // client wants to open `message.filename`, you must return a 'handle'
      // that represents that file, it can just be the filename
      // we're just going to give them the same file no matter what they
      // request
      openHandles['@' + message.filename] = fs.openSync('fileforyou.txt', 'r')
      message.replyHandle('@' + message.filename)
    })

    channel.on('sftp:read', function (message) {
      // client wants to read a chunk of the given handle that represents
      // an open file
      // sadly there is no easy streaming here, that's left up to you to
      // implement

      var buf = new Buffer(message.length) // message.length the reqeusted amount
      var length = fs.readSync(
            openHandles[message.handle]
          , buf
          , 0
          , message.length
          , message.offset // the requested start offset for a read
        )

      if (!length) // no more data left to read, send an EOF
        message.replyStatus('eof')
      else // `message.replyData()` needs a buffer and the length to send
        message.replyData(buf, length)
    })

    channel.on('sftp:close', function (message) {
      // client wants to close `message.handle`, tell it that it's done
      if (openHandles[message.handle]) {
        fs.closeSync(openHandles[message.handle])
        openHandles[message.handle] = undefined
      }
      message.replyStatus('ok')
    })
  })
})

server.listen(3333)
console.log('Listening on port 3333')