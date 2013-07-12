{'targets': [{
    'target_name': 'libssh'
  , 'variables': {
        'libsshversion': '0.5.90'
    }
  , 'type': 'static_library'
		# Overcomes an issue with the linker and thin .a files on SmartOS
  , 'standalone_static_library': 1
  , 'defines': [
        'SOURCEDIR=<!(pwd)/libssh-<(libsshversion)/'
      , 'BINARYDIR=<(PRODUCT_DIR)'
    ]
  , 'include_dirs': [
        'libssh-<(libsshversion)/include/'
      , 'include/'
    ]
  , 'direct_dependent_settings': {
        'include_dirs': [
            'libssh-<(libsshversion)/include/'
          , 'include/'
        ]
    }
  , 'sources': [
        'libssh-<(libsshversion)/src/agent.c'
      , 'libssh-<(libsshversion)/src/auth1.c'
      , 'libssh-<(libsshversion)/src/auth.c'
      , 'libssh-<(libsshversion)/src/base64.c'
      , 'libssh-<(libsshversion)/src/bind.c'
      , 'libssh-<(libsshversion)/src/buffer.c'
      , 'libssh-<(libsshversion)/src/callbacks.c'
      , 'libssh-<(libsshversion)/src/channels1.c'
      , 'libssh-<(libsshversion)/src/channels.c'
      , 'libssh-<(libsshversion)/src/client.c'
      , 'libssh-<(libsshversion)/src/config.c'
      , 'libssh-<(libsshversion)/src/connect.c'
      , 'libssh-<(libsshversion)/src/crc32.c'
      , 'libssh-<(libsshversion)/src/dh.c'
      , 'libssh-<(libsshversion)/src/ecdh.c'
      , 'libssh-<(libsshversion)/src/error.c'
      , 'libssh-<(libsshversion)/src/gcrypt_missing.c'
      , 'libssh-<(libsshversion)/src/getpass.c'
      , 'libssh-<(libsshversion)/src/gzip.c'
      , 'libssh-<(libsshversion)/src/init.c'
      , 'libssh-<(libsshversion)/src/kex1.c'
      , 'libssh-<(libsshversion)/src/kex.c'
      , 'libssh-<(libsshversion)/src/known_hosts.c'
      , 'libssh-<(libsshversion)/src/legacy.c'
      , 'libssh-<(libsshversion)/src/libcrypto.c'
      , 'libssh-<(libsshversion)/src/libgcrypt.c'
      , 'libssh-<(libsshversion)/src/log.c'
      , 'libssh-<(libsshversion)/src/match.c'
      , 'libssh-<(libsshversion)/src/messages.c'
      , 'libssh-<(libsshversion)/src/misc.c'
      , 'libssh-<(libsshversion)/src/options.c'
      , 'libssh-<(libsshversion)/src/packet1.c'
      , 'libssh-<(libsshversion)/src/packet.c'
      , 'libssh-<(libsshversion)/src/packet_cb.c'
      , 'libssh-<(libsshversion)/src/packet_crypt.c'
      , 'libssh-<(libsshversion)/src/pcap.c'
      , 'libssh-<(libsshversion)/src/pki.c'
      , 'libssh-<(libsshversion)/src/pki_crypto.c'
      , 'libssh-<(libsshversion)/src/pki_gcrypt.c'
      , 'libssh-<(libsshversion)/src/poll.c'
      , 'libssh-<(libsshversion)/src/scp.c'
      , 'libssh-<(libsshversion)/src/server.c'
      , 'libssh-<(libsshversion)/src/session.c'
      , 'libssh-<(libsshversion)/src/sftp.c'
      , 'libssh-<(libsshversion)/src/sftpserver.c'
      , 'libssh-<(libsshversion)/src/socket.c'
      , 'libssh-<(libsshversion)/src/string.c'
      , 'libssh-<(libsshversion)/src/threads.c'
      , 'libssh-<(libsshversion)/src/wrapper.c'
      , 'libssh-<(libsshversion)/src/threads/pthread.c'
    ]
}]}