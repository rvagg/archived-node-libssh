const test   = require('tap').test
    , fs     = require('fs')
    , bl     = require('bl')
    , Stat   = require('../').Stat
    , executeServerTest = require('./execute-server')
    , md5    = require('./util').md5

    , testfile = __dirname + '/testdata.bin'

// extend a vanilla libssh attr object to something node-ssh2 presents
function extendAttr (attr) {
  if (attr.permissions && !attr.mode)
    attr.mode = attr.permissions

  'uid gid size atime mtime'.split(' ').forEach(function (k) {
    if (typeof attr[k] == 'undefined')
      attr[k] = undefined
  })

  return attr
}

function makefakeattr (mode) {  
  return {
      mode: +mode
    , permissions: +mode
    , uid: 1010
    , gid: 2020
    , size: 3030
    , atime: 4040
    , mtime: 5050
  }
}

test('test sftp', function (t) {
  t.plan(executeServerTest.plan + 15)

  var connectOptions = {
          host: 'localhost'
        , port: 3333
        , username: 'foobar'
        , password: 'doobar'
      //, debug: console.error
      }
    , server

  function authCb (message) {
    return message.replyAuthSuccess()
  }

  function channelCb (channel) {
    var openHandles = {}

    channel.on('pty', function (message) {
      t.pass('triggered "pty" event')
      message.replySuccess()
    })
    channel.on('shell', function (message) {
      t.pass('triggered "shell" event')
      message.replySuccess()
    })
    channel.on('subsystem', function (message) {
      if (message.subsystem == 'sftp') {
        message.replySuccess()
        message.sftpAccept()
      }
    })
    channel.on('sftp:stat', function (message) {
      if (message.filename == testfile) {
        // special case prior to READ the test file
        var attr = makefakeattr(Stat(644).reg())
        attr.size = fs.statSync(testfile).size
        message.replyAttr(attr)
      } else {
        t.equal(message.filename, '.')
        message.replyAttr(makefakeattr(Stat('750').dir()))
      }
    })
    channel.on('sftp:lstat', function (message) {
      t.equal(message.filename, '/foo/bar/')
      message.replyAttr(makefakeattr(Stat('755').dir()))
    })
    channel.on('sftp:opendir', function (message) {
      t.equal(message.filename, '/baz/')
      message.replyHandle('/baz/:handle')
    })
    channel.on('sftp:close', function (message) {
      t.pass('got a sftp close')
      message.replyStatus('ok')
    })
    channel.on('sftp:open', function (message) {
      openHandles[message.filename] = fs.openSync(message.filename, 'r')
      message.replyHandle(message.filename)
    })
    channel.on('sftp:readdir', function (message) {
      t.equal(message.handle, '/baz/:handle')
      message.replyNames([
          { filename: 'foo', longname: 'foo', attrs: makefakeattr(Stat('644')) }
        , { filename: 'bar', longname: 'bar', attrs: makefakeattr(Stat('751')) }
        , { filename: 'baz', longname: 'baz', attrs: makefakeattr(Stat('620')) }
      ])
    })
    channel.on('sftp:read', function (message) {
      var buf = new Buffer(message.length)
        , length = fs.readSync(
              openHandles[message.handle]
            , buf
            , 0
            , message.length
            , message.offset
          )

      if (!length)
        message.replyStatus('eof')
      else
        message.replyData(buf, length)
    })
  }

  function readFile (connection, sftp) {
    var dstfile = __dirname + '/$$dstfile.' + Date.now()
    //sftp.
    sftp.fastGet(testfile, dstfile, { concurrency: 4, chunkSize: 100 }, function (err) {
      t.notOk(err, 'no error')
      t.equal(
          md5(fs.readFileSync(dstfile))
        , md5(fs.readFileSync(testfile))
        , 'same data!'
      )
      connection.end()
      server.close()
      fs.unlinkSync(dstfile)
      t.end()
    })
  }

  function connectionCb (connection) {
    connection.sftp(function (err, sftp) {
      t.notOk(err, 'no error')
      sftp.stat('.', function (err, result) {
        t.notOk(err, 'no error')
        t.deepEqual(result, makefakeattr(Stat('750').dir()), 'correct attributes for "stat ."')
        sftp.lstat('/foo/bar/', function (err, result) {
          t.notOk(err, 'no error')
          t.deepEqual(result, makefakeattr(Stat('755').dir()), 'correct attributes for "stat ."')
          sftp.opendir('/baz/', function (err, handle) {
            t.notOk(err, 'no error')
            t.equal(handle.toString(), '/baz/:handle', 'got correct handle') // arbitrary
            sftp.readdir(handle, function(err, list) {
              t.notOk(err, 'no error')
              t.deepEqual(list, [
                  { filename: 'foo', longname: 'foo', attrs: makefakeattr(Stat('644')) }
                , { filename: 'bar', longname: 'bar', attrs: makefakeattr(Stat('751')) }
                , { filename: 'baz', longname: 'baz', attrs: makefakeattr(Stat('620')) }
              ])
              sftp.close(handle, function () {
                readFile(connection, sftp)
              })
            });
          });
        })
      })
    })
  }

  server = executeServerTest(t, connectOptions, authCb, channelCb, connectionCb)
})