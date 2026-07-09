void setup(){
    //reset led array script w/ vibration
        //loop segments in a loop script w/ end if button tapped
}

void loop(){
    //ResetArray + idle animation
    //ResetPlay flash
    //PlayJack
        //Flash array
        //flash display 2 numbers
        //breathe idle until
        //Playblackjack
            //handle hit
            //handle stand
        //flash dispay 2 numbers
}


/*
  Blackjack for Arduino
  ---------------------
  Hardware:
    - Push button on pin 2 (other side to GND; uses internal pull-up)
    - Serial Monitor at 9600 baud (or any terminal)

  Controls:
    - Tap  (< 1 000 ms) = HIT
    - Hold (≥ 1 000 ms) = STAND

  Rules:
    - Standard Blackjack values (Ace = 11, auto-reduced to 1 if bust)
    - Dealer hits on soft 16 or less, stands on 17+
    - No split / double-down to keep memory footprint tiny
*/

// ── Pin & timing constants ────────────────────────────────────────────────────
const int    BTN_PIN      = 2;
const unsigned long HOLD_MS = 1000;   // hold threshold for STAND

// ── Game state ────────────────────────────────────────────────────────────────
int playerCards[10];   // up to 10 cards
int playerCount = 0;
int playerAces  = 0;

int dealerCards[10];
int dealerCount = 0;
int dealerAces  = 0;

bool gameOver = false;

// ── Button state ──────────────────────────────────────────────────────────────
bool         lastBtnState   = HIGH;   // HIGH = released (pull-up)
unsigned long pressStartMs  = 0;
bool         actionHandled  = false;  // prevents repeated fire while held

// ── Pseudo-random card draw (1-13, Ace=1 stored, face cards=10) ───────────────
int drawCard() {
  int val = (rand() % 13) + 1;   // 1..13
  if (val > 10) val = 10;        // J/Q/K → 10
  return val;                    // Ace stored as 1; handled via aces counters
}

// ── Score helpers ─────────────────────────────────────────────────────────────
void addCard(int cards[], int &count, int &aces, int card) {
  cards[count++] = card;
  if (card == 1) aces++;         // track aces for soft-hand promotion
}

int handScore(int cards[], int count, int aces) {
  int score = 0;
  for (int i = 0; i < count; i++) score += cards[i];
  // Promote aces from 1 → 11 as long as we don't bust
  int promoted = aces;
  while (promoted > 0 && score + 10 <= 21) {
    score += 10;
    promoted--;
  }
  return score;
}

// ── Print helpers ─────────────────────────────────────────────────────────────
void printCard(int card) {
  if      (card == 1)  Serial.print("A");
  else if (card == 10) Serial.print("10");  // could be 10/J/Q/K
  else                 Serial.print(card);
}

void printHand(const char* owner, int cards[], int count, int aces, bool hideSecond) {
  Serial.print(owner);
  Serial.print(": ");
  for (int i = 0; i < count; i++) {
    if (hideSecond && i == 1) { Serial.print("[?]"); }
    else                       { printCard(cards[i]); }
    if (i < count - 1) Serial.print(" ");
  }
  if (!hideSecond) {
    Serial.print("  (");
    Serial.print(handScore(cards, count, aces));
    Serial.print(")");
  }
  Serial.println();
}

// ── Deal initial hands ────────────────────────────────────────────────────────
void dealHands() {
  // Reset
  playerCount = playerAces = 0;
  dealerCount = dealerAces  = 0;

  // Two cards each, alternating
  addCard(playerCards, playerCount, playerAces, drawCard());
  addCard(dealerCards,  dealerCount, dealerAces,  drawCard());
  addCard(playerCards, playerCount, playerAces, drawCard());
  addCard(dealerCards,  dealerCount, dealerAces,  drawCard());
}

// ── Dealer plays automatically ────────────────────────────────────────────────
void dealerPlay() {
  Serial.println("\n-- Dealer's turn --");
  printHand("Dealer", dealerCards, dealerCount, dealerAces, false);

  while (handScore(dealerCards, dealerCount, dealerAces) <= 16) {
    int card = drawCard();
    addCard(dealerCards, dealerCount, dealerAces, card);
    Serial.print("Dealer draws: "); printCard(card); Serial.println();
    printHand("Dealer", dealerCards, dealerCount, dealerAces, false);
  }
}

// ── Determine winner ──────────────────────────────────────────────────────────
void checkWinner() {
  int ps = handScore(playerCards, playerCount, playerAces);
  int ds = handScore(dealerCards,  dealerCount,  dealerAces);

  Serial.println("\n===== RESULT =====");
  Serial.print("Your score : "); Serial.println(ps);
  Serial.print("Dealer score: "); Serial.println(ds);

  if (ps > 21) {
    Serial.println("BUST! You lose.");
  } else if (ds > 21) {
    Serial.println("Dealer busts! You WIN!");
  } else if (ps > ds) {
    Serial.println("You WIN!");
  } else if (ps < ds) {
    Serial.println("You LOSE.");
  } else {
    Serial.println("PUSH (tie).");
  }

  Serial.println("\n[Tap to play again]");
  gameOver = true;
}

// ── Start / restart a game ────────────────────────────────────────────────────
void startGame() {
  gameOver = false;
  Serial.println("\n==============================");
  Serial.println("        BLACKJACK");
  Serial.println("  TAP = Hit | HOLD = Stand");
  Serial.println("==============================");

  // Seed random with an analog noise read
  randomSeed(analogRead(A0));
  srand(analogRead(A0));

  dealHands();

  printHand("Dealer", dealerCards, dealerCount, dealerAces, true);   // hide hole card
  printHand("You   ", playerCards, playerCount, playerAces, false);

  // Instant blackjack?
  if (handScore(playerCards, playerCount, playerAces) == 21) {
    Serial.println("BLACKJACK!");
    dealerPlay();
    checkWinner();
  }
}

// ── Hit action ────────────────────────────────────────────────────────────────
void doHit() {
  int card = drawCard();
  addCard(playerCards, playerCount, playerAces, card);
  Serial.print("\nYou drew: "); printCard(card); Serial.println();
  printHand("You   ", playerCards, playerCount, playerAces, false);

  if (handScore(playerCards, playerCount, playerAces) > 21) {
    Serial.println("BUST!");
    dealerPlay();
    checkWinner();
  }
}

// ── Stand action ──────────────────────────────────────────────────────────────
void doStand() {
  Serial.println("\nYou STAND.");
  dealerPlay();
  checkWinner();
}

// ── Arduino setup ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  pinMode(BTN_PIN, INPUT_PULLUP);
  startGame();
}

// ── Arduino loop ──────────────────────────────────────────────────────────────
void loop() {
  bool btnState = digitalRead(BTN_PIN);   // LOW = pressed
  unsigned long now = millis();

  // ── Button pressed (falling edge) ──
  if (lastBtnState == HIGH && btnState == LOW) {
    pressStartMs = now;
    actionHandled = false;
  }

  // ── Button held ≥ HOLD_MS → STAND (fire once) ──
  if (btnState == LOW && !actionHandled) {
    if (now - pressStartMs >= HOLD_MS) {
      actionHandled = true;
      if (gameOver) {
        startGame();
      } else {
        doStand();
      }
    }
  }

  // ── Button released (rising edge) ──
  if (lastBtnState == LOW && btnState == HIGH) {
    if (!actionHandled) {
      // Short tap
      if (gameOver) {
        startGame();
      } else {
        doHit();
      }
    }
    actionHandled = false;
  }

  lastBtnState = btnState;
}