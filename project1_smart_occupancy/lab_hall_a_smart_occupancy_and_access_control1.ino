#include <LiquidCrystal.h>
#include <stdlib.h>

#define FACILITY_NAME       "Lab Hall A"
#define FACILITY_CAPACITY   15
#define EVENT_LOG_SIZE      12

#define PIN_BTN_ENTRY       6
#define PIN_BTN_EXIT        7
#define PIN_LED_GREEN       8
#define PIN_LED_RED         9

#define DEBOUNCE_MS         40UL
#define LCD_PERIOD_MS       250UL
#define STATS_PERIOD_MS     15000UL
#define BANNER_MS           2000UL

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

enum EventType {
    EVT_ENTRY_OK = 0,
    EVT_EXIT_OK,
    EVT_ENTRY_DENIED,
    EVT_EXIT_DENIED
};

struct AccessEvent {
    unsigned long timestampMs;
    EventType     type;
    uint8_t       occupancyAfter;
};

struct Facility {
    const char   *name;
    uint8_t       capacity;
    uint8_t       occupants;
    uint8_t       peak;
    uint16_t      entriesOk;
    uint16_t      exitsOk;
    uint16_t      entriesDenied;
    uint16_t      exitsDenied;

    AccessEvent  *log;
    uint8_t       logSize;
    uint8_t       logHead;
    uint8_t       logCount;
};

struct DebouncedButton {
    uint8_t       pin;
    bool          lastRaw;
    bool          stable;
    unsigned long lastFlipMs;
};

enum OpResult { OP_OK = 0, OP_FULL, OP_EMPTY };

Facility        *hall = NULL;
DebouncedButton  btnEntry = { PIN_BTN_ENTRY, HIGH, HIGH, 0 };
DebouncedButton  btnExit  = { PIN_BTN_EXIT,  HIGH, HIGH, 0 };

unsigned long lastLcdMs     = 0;
unsigned long lastStatsMs   = 0;
unsigned long bannerUntilMs = 0;
const char   *bannerText    = "";

struct Facility *facilityCreate(const char *name, uint8_t capacity, uint8_t logSize);
void             facilityDestroy(struct Facility **fp);
uint8_t          personEnters(struct Facility *f);
uint8_t          personLeaves(struct Facility *f);
uint8_t          freeSpaces(const struct Facility *f);
bool             isFull(const struct Facility *f);
void             logEvent(struct Facility *f, uint8_t type);
bool             buttonPressed(struct DebouncedButton *b, unsigned long now);
void             refreshLeds(const struct Facility *f);
void             refreshLcd(const struct Facility *f, unsigned long now);
void             showBanner(const char *text, unsigned long now);
void             printStats(const struct Facility *f);
void             printEventLog(const struct Facility *f);
void             printHelp(void);
void             handleSerialCommand(unsigned long now);
const char      *eventName(uint8_t t);
void             fatalError(const char *msg);

void setup()
{
    Serial.begin(9600);

    pinMode(PIN_BTN_ENTRY, INPUT_PULLUP);
    pinMode(PIN_BTN_EXIT,  INPUT_PULLUP);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_RED,   OUTPUT);

    lcd.begin(16, 2);

    hall = facilityCreate(FACILITY_NAME, FACILITY_CAPACITY, EVENT_LOG_SIZE);
    if (hall == NULL)
        fatalError("malloc failed - cannot create facility record");

    Serial.println(F("=========================================="));
    Serial.println(F(" SMART OCCUPANCY & ACCESS CONTROL - READY"));
    Serial.println(F("=========================================="));
    Serial.print(F("Facility : ")); Serial.println(hall->name);
    Serial.print(F("Capacity : ")); Serial.println(hall->capacity);
    printHelp();

    refreshLeds(hall);
    refreshLcd(hall, millis());
}

void loop()
{
    unsigned long now = millis();

    if (buttonPressed(&btnEntry, now)) {
        if (personEnters(hall) == OP_OK) {
            Serial.print(F("[ENTRY] Accepted. Inside: "));
            Serial.print(hall->occupants);
            Serial.print(F(" | Free: "));
            Serial.println(freeSpaces(hall));
        } else {
            Serial.println(F("[ERROR] Entry rejected - facility is at MAXIMUM capacity."));
            showBanner("FULL! NO ENTRY  ", now);
        }
        refreshLeds(hall);
    }

    if (buttonPressed(&btnExit, now)) {
        if (personLeaves(hall) == OP_OK) {
            Serial.print(F("[EXIT ] Accepted. Inside: "));
            Serial.print(hall->occupants);
            Serial.print(F(" | Free: "));
            Serial.println(freeSpaces(hall));
        } else {
            Serial.println(F("[ERROR] Exit rejected - nobody is inside (count is 0)."));
            showBanner("EMPTY! NO EXIT  ", now);
        }
        refreshLeds(hall);
    }

    handleSerialCommand(now);

    if (now - lastLcdMs >= LCD_PERIOD_MS) {
        lastLcdMs = now;
        refreshLcd(hall, now);
    }

    if (now - lastStatsMs >= STATS_PERIOD_MS) {
        lastStatsMs = now;
        printStats(hall);
    }
}

struct Facility *facilityCreate(const char *name, uint8_t capacity, uint8_t logSize)
{
    Facility *f = (Facility *)malloc(sizeof(Facility));
    if (f == NULL)
        return NULL;

    f->log = (AccessEvent *)calloc(logSize, sizeof(AccessEvent));
    if (f->log == NULL) {
        free(f);
        return NULL;
    }

    f->name          = name;
    f->capacity      = capacity;
    f->occupants     = 0;
    f->peak          = 0;
    f->entriesOk     = 0;
    f->exitsOk       = 0;
    f->entriesDenied = 0;
    f->exitsDenied   = 0;
    f->logSize       = logSize;
    f->logHead       = 0;
    f->logCount      = 0;
    return f;
}

void facilityDestroy(struct Facility **fp)
{
    if (fp == NULL || *fp == NULL)
        return;
    free((*fp)->log);
    (*fp)->log = NULL;
    free(*fp);
    *fp = NULL;
}

uint8_t personEnters(struct Facility *f)
{
    if (isFull(f)) {
        f->entriesDenied++;
        logEvent(f, EVT_ENTRY_DENIED);
        return OP_FULL;
    }
    f->occupants++;
    f->entriesOk++;
    if (f->occupants > f->peak)
        f->peak = f->occupants;
    logEvent(f, EVT_ENTRY_OK);
    return OP_OK;
}

uint8_t personLeaves(struct Facility *f)
{
    if (f->occupants == 0) {
        f->exitsDenied++;
        logEvent(f, EVT_EXIT_DENIED);
        return OP_EMPTY;
    }
    f->occupants--;
    f->exitsOk++;
    logEvent(f, EVT_EXIT_OK);
    return OP_OK;
}

uint8_t freeSpaces(const struct Facility *f)
{
    return f->capacity - f->occupants;
}

bool isFull(const struct Facility *f)
{
    return f->occupants >= f->capacity;
}

void logEvent(struct Facility *f, uint8_t type)
{
    AccessEvent *slot = f->log + f->logHead;
    slot->timestampMs    = millis();
    slot->type           = (EventType)type;
    slot->occupancyAfter = f->occupants;

    f->logHead = (f->logHead + 1) % f->logSize;
    if (f->logCount < f->logSize)
        f->logCount++;
}

bool buttonPressed(struct DebouncedButton *b, unsigned long now)
{
    bool raw = digitalRead(b->pin);

    if (raw != b->lastRaw) {
        b->lastRaw    = raw;
        b->lastFlipMs = now;
        return false;
    }

    if ((now - b->lastFlipMs) >= DEBOUNCE_MS && raw != b->stable) {
        b->stable = raw;
        return (b->stable == LOW);
    }
    return false;
}

void refreshLeds(const struct Facility *f)
{
    bool full = isFull(f);
    digitalWrite(PIN_LED_GREEN, full ? LOW  : HIGH);
    digitalWrite(PIN_LED_RED,   full ? HIGH : LOW);
}

void showBanner(const char *text, unsigned long now)
{
    bannerText    = text;
    bannerUntilMs = now + BANNER_MS;
    refreshLcd(hall, now);
}

void refreshLcd(const struct Facility *f, unsigned long now)
{
    char line[17];

    snprintf(line, sizeof(line), "Inside: %u/%u", f->occupants, f->capacity);
    lcd.setCursor(0, 0);
    lcd.print(line);
    for (uint8_t i = strlen(line); i < 16; i++) lcd.print(' ');

    lcd.setCursor(0, 1);
    if (bannerUntilMs != 0 && (long)(bannerUntilMs - now) > 0) {
        lcd.print(bannerText);
        return;
    }
    bannerUntilMs = 0;

    snprintf(line, sizeof(line), "Free  : %-2u %s",
             freeSpaces(f), isFull(f) ? "FULL" : "OPEN");
    lcd.print(line);
    for (uint8_t i = strlen(line); i < 16; i++) lcd.print(' ');
}

void printStats(const struct Facility *f)
{
    Serial.println(F("------------- OCCUPANCY STATISTICS -------------"));
    Serial.print(F("Facility         : ")); Serial.println(f->name);
    Serial.print(F("Currently inside : ")); Serial.print(f->occupants);
    Serial.print(F(" / "));                 Serial.println(f->capacity);
    Serial.print(F("Remaining space  : ")); Serial.println(freeSpaces(f));
    Serial.print(F("Utilisation      : "));
    Serial.print((100UL * f->occupants) / f->capacity); Serial.println(F(" %"));
    Serial.print(F("Peak occupancy   : ")); Serial.println(f->peak);
    Serial.print(F("Entries accepted : ")); Serial.println(f->entriesOk);
    Serial.print(F("Exits accepted   : ")); Serial.println(f->exitsOk);
    Serial.print(F("Entries rejected : ")); Serial.println(f->entriesDenied);
    Serial.print(F("Exits rejected   : ")); Serial.println(f->exitsDenied);
    Serial.print(F("Status           : "));
    Serial.println(isFull(f) ? F("FULL (red LED)") : F("AVAILABLE (green LED)"));
    Serial.println(F("------------------------------------------------"));
}

void printEventLog(const struct Facility *f)
{
    Serial.println(F("--------------- RECENT EVENTS ------------------"));
    if (f->logCount == 0) {
        Serial.println(F("(no events yet)"));
        return;
    }
    uint8_t start = (f->logHead + f->logSize - f->logCount) % f->logSize;
    for (uint8_t i = 0; i < f->logCount; i++) {
        const AccessEvent *e = &f->log[(start + i) % f->logSize];
        Serial.print(F("t="));
        Serial.print(e->timestampMs / 1000.0, 1);
        Serial.print(F("s  "));
        Serial.print(eventName(e->type));
        Serial.print(F("  -> inside="));
        Serial.println(e->occupancyAfter);
    }
    Serial.println(F("------------------------------------------------"));
}

void printHelp(void)
{
    Serial.println(F("Commands: s=stats  l=event log  r=reset  h=help"));
}

void handleSerialCommand(unsigned long now)
{
    if (!Serial.available())
        return;

    char c = Serial.read();
    switch (c) {
    case 's': case 'S': printStats(hall);    break;
    case 'l': case 'L': printEventLog(hall); break;
    case 'h': case 'H': printHelp();         break;
    case 'r': case 'R':
        facilityDestroy(&hall);
        hall = facilityCreate(FACILITY_NAME, FACILITY_CAPACITY, EVENT_LOG_SIZE);
        if (hall == NULL)
            fatalError("malloc failed during reset");
        Serial.println(F("[RESET] Records released and re-allocated. Count = 0."));
        refreshLeds(hall);
        refreshLcd(hall, now);
        break;
    case '\n': case '\r': case ' ':
        break;
    default:
        Serial.print(F("[ERROR] Unknown command '"));
        Serial.print(c);
        Serial.println(F("'. Type h for help."));
    }
}

const char *eventName(uint8_t t)
{
    switch (t) {
    case EVT_ENTRY_OK:     return "ENTRY  accepted";
    case EVT_EXIT_OK:      return "EXIT   accepted";
    case EVT_ENTRY_DENIED: return "ENTRY  REJECTED (full)";
    case EVT_EXIT_DENIED:  return "EXIT   REJECTED (empty)";
    }
    return "UNKNOWN";
}

void fatalError(const char *msg)
{
    Serial.print(F("[FATAL] "));
    Serial.println(msg);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED, HIGH);
    lcd.clear();
    lcd.print("SYSTEM ERROR");
    lcd.setCursor(0, 1);
    lcd.print("Memory failure");
    for (;;) {   }
}
