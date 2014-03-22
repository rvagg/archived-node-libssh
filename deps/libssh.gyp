{'targets': [{
    'target_name': 'libssh'
  , 'variables': {
        'libsshversion': '0.6.3'
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
    ]
  , 'direct_dependent_settings': {
        'include_dirs': [
            'libssh-<(libsshversion)/include/'
        ]
    }
  , 'conditions': [
        ['node_shared_openssl=="false"', {
          'include_dirs': [
            '<(node_root_dir)/deps/openssl/openssl/include'
          ],
          "conditions" : [
            ["target_arch=='ia32'", {
              "include_dirs": [ "<(node_root_dir)/deps/openssl/config/piii" ]
            }],
            ["target_arch=='x64'", {
              "include_dirs": [ "<(node_root_dir)/deps/openssl/config/k8" ]
            }],
            ["target_arch=='arm'", {
              "include_dirs": [ "<(node_root_dir)/deps/openssl/config/arm" ]
            }]
          ]
        }]
      , ['node_shared_zlib=="false"', {
          'include_dirs': [
            '<(node_root_dir)/deps/zlib'
          ]
       }]
     , ['OS == "mac"', {
           'include_dirs': [
               'include-osx/'
           ]
         , 'direct_dependent_settings': {
               'include_dirs': [
                   'include-osx/'
               ]
           }
       }, { # OS != "mac"
           'include_dirs': [
               'include/'
           ]
         , 'direct_dependent_settings': {
               'include_dirs': [
                   'include/'
               ]
           }
       }]
    ]
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
      , 'libssh-<(libsshversion)/src/curve25519.c'
      , 'libssh-<(libsshversion)/src/curve25519_ref.c'
      , 'libssh-<(libsshversion)/src/dh.c'
      , 'libssh-<(libsshversion)/src/ecdh.c'
      , 'libssh-<(libsshversion)/src/error.c'
      , 'libssh-<(libsshversion)/src/gcrypt_missing.c'
      , 'libssh-<(libsshversion)/src/getpass.c'
      , 'libssh-<(libsshversion)/src/gssapi.c'
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
    ]
}]}