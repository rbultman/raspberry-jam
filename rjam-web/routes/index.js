var express = require('express');
var router = express.Router();
var blynkToken = require('./blynkToken');

/* GET home page. */
router.get('/', function(req, res, next) {
  res.render('index', { title: 'Raspberry Jam', blynkToken: blynkToken.get() });
});

router.post('/', (req, res) => {
  blynkToken.set(req.body.blynkToken);
  res.render('thanks', { title: 'Configuration form' });
});

module.exports = router;
