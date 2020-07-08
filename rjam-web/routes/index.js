//jshint esversion: 8
var express = require('express');
var router = express.Router();
var blynkToken = require('./blynkToken');
var soundcards = require('./soundcards');

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

router.post('/', (req, res) => {
  var cards = soundcards.getCards();
  blynkToken.set(req.body.blynkToken);
  console.log("Request: " + JSON.stringify(req.body));
  cardSelected = req.body.cardSelect;
  for (var i=0; i<cards.length; i++) {
    if (cardSelected === cards[i].name) {
      console.log("Selected card is " + cards[i]. name + " and it's ID is " + cards[i].id);
      soundcards.setSelectedCard(cards[i].id);
      break;
    }
  }
  res.render('thanks', { title: 'Configuration form' });
});

module.exports = router;
