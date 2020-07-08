//jshint esversion: 8
var express = require('express');
var router = express.Router();
var blynkToken = require('./blynkToken');
var soundcards = require('./soundcards');
const { exec } = require("child_process");

/* GET home page. */
router.get('/', function(req, res, next) {
  var cards = soundcards.getCards();
  var options = [];
  var selectedCard = soundcards.getSelectedCard();
  for (var i=0; i<cards.length; i++) {
    options.push(cards[i].name);
  }
  console.log("The options list: " + options);
  res.render('index', { title: 'Raspberry Jam', blynkToken: blynkToken.get(), cards: options, selectedCardId: selectedCard });
});

function rebootJam() {
  exec('sudo reboot', (error, stdout, stderr) => {
    if (error) {
      console.log(`error: ${error.message}`);
      return;
    }
    if (stderr) {
      console.log(`stderr: ${stderr}`);
      return;
    }
    console.log(`stdout: ${stdout}`);
    console.log("Rebooting...");
  });
}

router.post('/', (req, res) => {
  var cards = soundcards.getCards();
  // save the token
  blynkToken.set(req.body.blynkToken);
  console.log("Request: " + JSON.stringify(req.body));
  cardSelected = req.body.cardSelect;
  for (var i=0; i<cards.length; i++) {
    if (cardSelected === cards[i].name) {
      console.log("Selected card is " + cards[i]. name + " and it's ID is " + cards[i].id);
      // update the soundcard
      soundcards.setSelectedCard(cards[i].id);
      break;
    }
  }
  // show the thanks page
  res.render('thanks', { title: 'Configuration form' });
  // reboot for changes to take effect
  setTimeout(rebootJam, 2000);
});

module.exports = router;
