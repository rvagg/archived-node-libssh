//TODO: perhaps just use process.binding('constants') to do some of this

function Stat (initial) {
  if (!(this instanceof Stat))
    return new Stat(initial)

  this.stat = typeof initial == 'number' ? initial : 0
  if (typeof initial == 'string')
    this.stat = parseInt(initial, 8)
}

Stat.prototype.valueOf = function () {
  return this.stat
}

Stat.prototype.toString = function () {
  return this.stat.toString(8)
}

var stats = {
    IFDIR:   040000 // Directory
  , IFCHR:   020000 // Character device
  , IFBLK:   060000 // Block device
  , IFREG:  0100000 // Regular file
  , IFIFO:   010000 // FIFO
  , IFLNK:  0120000 // Symbolic link
  , IFSOCK: 0140000 // Socket
  , ISUID:    04000 // Set user ID on execution
  , ISGID:    02000 // Set group ID on execution
  , ISVTX:    01000 // Save swapped text after use (sticky)
  , IRUSR:     0400 // Read by user
  , IWUSR:     0200 // Write by user
  , IXUSR:     0100 // Execute by user
  , IRGRP:      040 // Read by group
  , IWGRP:      020 // Write by group
  , IXGRP:      010 // Execute by group
  , IROTH:       04 // Read by others
  , IWOTH:       02 // Write by others
  , IXOTH:       01 // Execute by others
}

Object.keys(stats).forEach(function (k) {
  Stat[k] = stats[k]
  var f = k.toLowerCase().replace(/^if?/, '')
  Stat.prototype[f] = function () {
    this.stat |= stats[k]
    return this
  }
})

module.exports = Stat