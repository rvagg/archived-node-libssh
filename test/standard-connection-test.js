const test    = require('tap').test
    , fs      = require('fs')
    , bl      = require('bl')
    , executeServerTest = require('./execute-server')
    , md5    = require('./util').md5

    , testfile = __dirname + '/testdata.bin'

    , privkey = fs.readFileSync(__dirname + '/keys/id_rsa')
    , pubkey  = fs.readFileSync(__dirname + '/keys/id_rsa.pub')


// priv/pub key auth + pipe a file in to the session and verify it got there
test('test standard pub/privkey connection', function (t) {
  t.plan(executeServerTest.plan + 6)

  var connectOptions = {
      host: 'localhost'
    , port: 3333
    , username: 'foobar'
    , privateKey: privkey
    //, debug: console.error
  }

  function authCb (message) {
    if (message.comparePublicKey(pubkey))
      return message.replyAuthSuccess()

    t.fail('comparePublicKey did not work!')
  }

  function channelCb (channel) {
    channel.on('pty', function (message) {
      t.pass('triggered "pty" event')
      message.replySuccess()
    })
    channel.on('shell', function (message) {
      t.pass('triggered "shell" event')
      message.replySuccess()
    })
    channel.pipe(bl(function (err, data) {
      t.notOk(err, 'no error')
      t.equal(
          md5(data.slice())
        , md5(fs.readFileSync(testfile))
        , 'same data!'
      )
      channel.end()
    }))
  }

  function connectionCb (connection) {
    connection.shell(function (err, stream) {
      t.notOk(err, 'no error')
      t.ok(stream, 'has stream')
      fs.createReadStream(testfile)
        .on('close', function () {
          connection.end()
        })
        .pipe(stream)
    })
  }

  executeServerTest(t, connectOptions, authCb, channelCb, connectionCb)
})

// password auth + with stream going the other way
test('test standard password connection', function (t) {
  t.plan(executeServerTest.plan + 8)

  var connectOptions = {
          host: 'localhost'
        , port: 3333
        , username: 'foobar'
        , password: 'doobar'
        //, debug: console.error
      }
    , server
    , connection
    , clientchannel

  function authCb (message) {
    if (message.authUser === 'foobar' && message.authPassword === 'doobar')
      return message.replyAuthSuccess()
    t.fail('comparePublicKey did not work!')
  }

  function channelCb (channel) {
    channel.on('pty', function (message) {
      t.pass('triggered "pty" event')
      message.replySuccess()
    })
    channel.on('shell', function (message) {
      t.pass('triggered "shell" event')
      message.replySuccess()

      fs.createReadStream(testfile)
        .on('close', function () {
          setTimeout(function () {
            channel.close()
          }, 100)
          setTimeout(connection.end.bind(connection), 200)
        })
        .pipe(channel)
      channel.pipe(bl(function (err, data) {
        t.notOk(err, 'no error')
        t.equal(data.length, 0, 'got no data on read')
      }))
    })
  }

  function connectionCb (_connection) {
    connection = _connection
    connection.shell(function (err, stream) {
      clientchannel = stream
      t.notOk(err, 'no error')
      t.ok(stream, 'has stream')

      stream.pipe(bl(function (err, data) {
        t.notOk(err, 'no error')
        t.equal(
            md5(data.slice())
          , md5(fs.readFileSync(testfile))
          , 'same data!'
        )
      }))
    })
  }

  server = executeServerTest(t, connectOptions, authCb, channelCb, connectionCb)
})