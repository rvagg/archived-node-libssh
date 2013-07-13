const crypto = require('crypto')

module.exports.md5 = function (b) {
  var md5sum = crypto.createHash('md5');
  md5sum.update(b)
  return md5sum.digest('hex')
}