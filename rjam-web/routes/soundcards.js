//jshint esversion: 8
var sqlite3 = require('sqlite3');
var sqlite = require('sqlite');
var open = sqlite.open;

var db;
var cards = [];
var selected = -1;
 
(async () => {
  // open the database
  try {
    db = await open('../settings/sessions.db');
    getCardsFromDatabase();
    getSelectedCardFromDatabase();
  } catch(e) {
    console.log("Error opening database: " + e);
  }
})();

async function getCardsFromDatabase() {
  try {
    const result = await db.all('select * from soundcards order by id');
    cards = result;
  } catch(e) {
    console.log("Error getting cards from database: ", e);
    cards = [];
  }
}

async function getSelectedCardFromDatabase() {
  try {
    const result = await db.all('select selectedCard from settings where id=1');
    console.log("Selected: " + JSON.stringify(result));
    if (result.length > 0) {
      selected = result[0].selectedCard;
      console.log("Selected card is " + selected);
    } else {
      selected = -1;
    }
  } catch(e) {
    console.log("Error getting selected card from database: ", e);
    selected = -1;
  }
}

async function updateSelectedCardInDatabase(card) {
  try {
    const result = await db.all('update settings set selectedCard=' + card + ' where id=1');
    console.log('Selected card updated successfully.');
    selected = card;
  } catch(e) {
    console.log("Error setting selected card in database: ", e);
    selected = -1;
  }
}

function getCards() {
  return cards;
}

function getSelectedCard() {
  console.log("getSelectedCard called");
  return selected;
}
  
function setSelectedCard(card) {
  console.log("setSelectedCard called");
  console.log("Requested selection: " + card);
  updateSelectedCardInDatabase(card);
}

var interface = {
  getCards: getCards,
  getSelectedCard: getSelectedCard,
  setSelectedCard: setSelectedCard
};

module.exports = interface;
