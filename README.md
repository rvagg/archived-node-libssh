node-libssh
===========

A Low-level Node.js binding for [libssh](http://www.libssh.org/)
----------------------------------------------------------------

[![Build Status](https://secure.travis-ci.org/rvagg/node-libssh.png)](http://travis-ci.org/rvagg/node-libssh)

Currently this project is only concerned with a subset of the **server** functionality provided by libssh. The client functionality may be added at a future date (and you're invited to contribute if you want it!).

You can find it in npm as **ssh**, (version 0.0.1 of which is [substack's version](https://github.com/substack/node-ssh) with an older libssh).

[![NPM](https://nodei.co/npm/ssh.png?downloads)](https://nodei.co/npm/ssh/)

### Installing

To compile, you'll need to have libkrb5-dev (kerberos development files) and libssl-dev (openssl development files) installed on your system. `npm install ssh` should do the rest.

### Lets make a Node.js SSH server!

```js
var server = libssh.createServer({
    hostRsaKeyFile : '/path/to/host_rsa'
  , hostDsaKeyFile : '/path/to/host_dsa'
})

server.on('connection', function (session) {
  session.on('auth', function (message) {
    if (message.subtype == 'publickey'
        && message.authUser == '$ecretb@ckdoor'
        && message.comparePublicKey(
            fs.readFileSync('/path/to/id_rsa.pub'))) {
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
```

See *[stdiopipe.js](https://github.com/rvagg/node-libssh/blob/master/examples/stdiopipe.js)* in the examples directory if you want to try this out.

### Remote exec!

We can receive **exec** requests and send the results back to the client. In this example we'll allow *any* exec if you have the right publickey.

```js
// a simple exec utility that spawns a process and pipes stdio to
// back to the channel
function exec (channel, cmd) {
  var cmdarr = cmd.split(' ')
    , child  = spawn(cmdarr.shift(), cmdarr)

  child.stdout.pipe(channel)

  child.on('close', function (code) {
    // explicitly end the command with an EOF and send the exit status
    channel.sendEof()
    channel.sendExitStatus(code)
    channel.close()
  })
}

server.on('connection', function (session) {
  session.on('auth', function (message) {
    if (message.subtype == 'publickey'
        && message.comparePublicKey(
            fs.readFileSync(__dirname + '/path/to/id_rsa.pub'))) {
      // could check message.authUser if we cared about the username
      return message.replyAuthSuccess()
    }
    message.replyDefault() // auth failed
  })

  session.on('channel', function (channel) {
    channel.on('exec', function (message) {
      message.replySuccess() // a success reply is needed before we send output
      exec(channel, message.execCommand)
    })
  })
})
```

See *[exec.js](https://github.com/rvagg/node-libssh/blob/master/examples/exec.js)* in the examples directory if you want to try this out.

### How about some SFTP goodness?

```js
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
```

See *[trickysftp.js](https://github.com/rvagg/node-libssh/blob/master/examples/trickysftp.js)* in the examples directory if you want to try this out.

SFTP events include:

 * sftp:open
 * sftp:close
 * sftp:read
 * sftp:write
 * sftp:lstat
 * sftp:fstat
 * sftp:setstat
 * sftp:fsetstat
 * sftp:opendir
 * sftp:readdir
 * sftp:remove
 * sftp:mkdir
 * sftp:rmdir
 * sftp:realpath
 * sftp:stat
 * sftp:rename
 * sftp:readlink
 * sftp:symlink

See the test files for more usage examples.


### `Stat`

*TODO: document this...*

## Important project notes

This project is very new and immature and is bound to have some warts. There are a few known, minor memory leaks that need to be addressed. While node-libssh makes use of both libssh's nonblocking I/O facilities and libuv's socket polling, it's likely that there could be more performance gained from some more async work within the binding code.

The streams do not implement back-pressure very well, particularly the read component of channel stream which will just keep on filling up its buffer.

Please file issues if you have any questions or concerns or want to see a particular area focused on for development&mdash;just don't expect me to be able to justify time developing or fixing your own pet features, contributions would be greatly appreciated no matter how much of a n00b you feel.

If you want to see more of what's going on, you can send a `debug:true` option when you make a new `Server` instance, it'll print out some message details. There's additional debug cruft you can enable in the source but you'll have to dig to find that and it's very noisy.

<a name="contributing"></a>
Contributing
------------

node-libssh is an **OPEN Open Source Project**. This means that:

> Individuals making significant and valuable contributions are given commit-access to the project to contribute as they see fit. This project is more like an open wiki than a standard guarded open source project.

See the [CONTRIBUTING.md](https://github.com/rvagg/node-libssh/blob/master/CONTRIBUTING.md) file for more details.

<a name="maintainers"></a>
Maintainers
-----------
 * Rod Vagg [@rvagg](https://github.com/rvagg)
 * Brian White [@mscdex](https://github.com/mscdex)
 * Audrius Butkevicius
   [@AudriusButkevicius](https://github.com/AudriusButkevicius)
 * Darius Clark [@dariusc93](https://github.com/dariusc93)


<a name="licence"></a>
Licence &amp; copyright
-------------------

Copyright (c) 2013-2014 Rod Vagg and Maintainers (above)

node-libssh is licensed under an MIT +no-false-attribs license. All rights not explicitly granted in the MIT license are reserved. See the included LICENSE file for more details.

*node-libssh builds on the excellent work of the [libssh](http://www.libssh.org/) team. **libssh** is licensed under the LGPLv2.
