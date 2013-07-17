const test    = require('tap').test
    , fs      = require('fs')
    , bl      = require('bl')
    , executeServerTest = require('./execute-server')

    , privkey = fs.readFileSync(__dirname + '/keys/id_rsa')


test('test exec test', function (t) {
  t.plan(executeServerTest.plan + 7)

  var connectOptions = {
      host: 'localhost'
    , port: 3333
    , username: 'foobar'
    , privateKey: privkey
  }

  function authCb (message) {
    return message.replyAuthSuccess()
  }

  function channelCb (channel) {
    channel.on('exec', function (message) {
      t.equal(message.execCommand, 'cat /etc/shadow')
      channel.pipe(bl(function (err, buf) {
        t.notOk(err, 'no error')
        t.equal(buf.length, 0, 'no data from other side')
      }))
      message.replySuccess()
      channel.write('foobar\n')
      channel.write('doobar\n')
      channel.sendEof()
      channel.sendExitStatus(0)
      channel.close()
    })
  }

  function connectionCb (connection) {
    connection.exec('cat /etc/shadow', function (err, stream) {
      t.notOk(err, 'no error')
      t.ok(stream, 'has stream')
      stream.pipe(bl(function (err, buf) {
        t.notOk(err, 'no error')
        t.equal(buf.toString(), 'foobar\ndoobar\n', 'got correct exec output')
        stream.end()
        setTimeout(function () {
          connection.end()
        }, 500)
      }))
    })
  }

  executeServerTest(t, connectOptions, authCb, channelCb, connectionCb)
})