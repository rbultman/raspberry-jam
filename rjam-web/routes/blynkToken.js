var fs = require("fs");

function get() {
  var token = "";
  try {
    var b = fs.readFileSync('blynk-token.txt');
    token = b.toString();
  } catch(err) {
    console.log("Error reading the token from the file: " + err);
  } finally {
    return token;
  }
}

function set(t) {
  fs.open('blynk-token.txt', "w", (err, fd) => {
    if (err) {
      console.log("Error opening token file for writing: " + err);
    } else {
      fs.write(fd, t, (err, bytesWritten, s) => {
        if (err) {
          console.log("Error writing to file: " + err);
        } else {
        }
      });
      fs.close(fd);
    }
  });
}

var interface = {
  get: get,
  set: set
};

module.exports = interface;
