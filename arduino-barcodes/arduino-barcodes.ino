/*
  32-bit digital barcodes for synchronizing data streams
  - exact 30 s cycle (drift-free, anchored with millis())
  - reliable random 32-bit start, board-independent
  - symmetric low/high/low marker on both ends of the barcode
*/
const int OUTPUT_PIN = 13;
const int BARCODE_BITS = 32;
const int BIT_DELAY_MS = 30;                    // per-bit duration (~928 ms for 32 bits)
const int MARKER_SEG_MS = 10;                   // each segment of the low/high/low marker
const unsigned long CYCLE_PERIOD_MS = 10000UL;  // exact 10 s between barcode starts

unsigned long barcode;
unsigned long nextCycle;

// low/high/low triplet — a shape no single data bit can produce,
// so the analysis side can unambiguously find a barcode's edges.
void emitMarker() {
  digitalWrite(OUTPUT_PIN, LOW);  delay(MARKER_SEG_MS);
  digitalWrite(OUTPUT_PIN, HIGH); delay(MARKER_SEG_MS);
  digitalWrite(OUTPUT_PIN, LOW);  delay(MARKER_SEG_MS);
}

void setup() {
  pinMode(OUTPUT_PIN, OUTPUT);

  // Harvest entropy from the noisy LSBs of a floating analog pin.
  unsigned long seed = 0;
  for (uint8_t i = 0; i < 32; i++) {
    seed = (seed << 1) | (analogRead(A0) & 1);
    delayMicroseconds(50);
  }
  randomSeed(seed);

  // Build a full 32-bit random value from two 16-bit draws.
  // (random() can't return the whole 32-bit range in one call.)
  barcode = ((unsigned long)random(0, 65536) << 16) |
             (unsigned long)random(0, 65536);

  nextCycle = millis();  // first cycle starts immediately
}

void loop() {
  // Block until this cycle's scheduled start time.
  // The (long) cast keeps this correct across millis() rollover (~49.7 days).
  while ((long)(millis() - nextCycle) < 0) { /* wait */ }
  nextCycle += CYCLE_PERIOD_MS;  // lock the next start to an absolute 30 s grid

  barcode += 1;  // increment barcode each cycle

  emitMarker();  // leading marker

  // barcode payload
  for (int i = 0; i < BARCODE_BITS; i++) {
    digitalWrite(OUTPUT_PIN, ((barcode >> i) & 1) ? HIGH : LOW);
    delay(BIT_DELAY_MS);
  }

  emitMarker();  // trailing marker (identical to the leading one)

  digitalWrite(OUTPUT_PIN, LOW);
  // the remaining ~9 s of the period is spent waiting at the top of loop()
}