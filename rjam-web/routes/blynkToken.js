//jshint esversion: 8
var fs = require("fs");
const filename = "../settings/blynk-token.txt";

function get() {
  var token = "";
  try {
    var b = fs.readFileSync(filename);
    token = b.toString();
  } catch(err) {
    console.log("Error reading the token from the file: " + err);
  } finally {
    return token;
  }
}

function set(t) {
  fs.open(filename, "w", (err, fd) => {
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
