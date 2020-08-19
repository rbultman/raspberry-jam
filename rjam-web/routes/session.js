//jshint esversion: 8
var express = require('express');
var router = express.Router();
const { exec } = require("child_process");

/* GET home page. */
router.get('/', function(req, res, next) {
  res.render('session');
});

module.exports = router;
