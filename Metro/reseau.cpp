#include "reseau.h"

#include <stdio.h>
#include <string.h>

#include "reseau_data.h"
#include "sncf_list_data.h"

static const int LINE_COUNT = sizeof(NET_LINES) / sizeof(NET_LINES[0]);
static const int PICK_COUNT = sizeof(SNCF_PICKS) / sizeof(SNCF_PICKS[0]);

int netLineCount() { return LINE_COUNT; }

const NetLine& netLine(int index) { return NET_LINES[index < 0 || index >= LINE_COUNT ? 0 : index]; }

const LineInfo* netLineInfo(int index) { return findLine(netLine(index).id); }

int netLinesOfMode(LineMode mode, int* out, int maxN) {
  int n = 0;
  for (int i = 0; i < LINE_COUNT && n < maxN; i++) {
    const LineInfo* li = netLineInfo(i);
    if (li && li->mode == mode) out[n++] = i;
  }
  return n;
}

const NetStop& netStop(int line, int i) {
  const NetLine& l = netLine(line);
  if (i < 0 || i >= l.count) i = 0;
  return NET_STOPS[l.first + i];
}

const char* netStopName(int line, int i) { return NET_NAMES + netStop(line, i).name; }

void netStopRef(int line, int i, char* out, size_t n) {
  snprintf(out, n, "STIF:StopArea:SP:%lu:", (unsigned long)netStop(line, i).zda);
}

// ----------------------------------------------------------------------------
//  Gares SNCF
// ----------------------------------------------------------------------------
// Initiale sans accent (les noms sont en UTF-8)
static char initial(const char* s) {
  const unsigned char c = (unsigned char)s[0];
  if (c == 0xC3) {
    const unsigned char d = (unsigned char)s[1];
    if (d >= 0x80 && d <= 0x85) return 'A';
    if (d == 0x87) return 'C';
    if (d >= 0x88 && d <= 0x8B) return 'E';
    if (d >= 0x8C && d <= 0x8F) return 'I';
    if (d >= 0x92 && d <= 0x96) return 'O';
    if (d >= 0x99 && d <= 0x9C) return 'U';
  }
  return (c >= 'a' && c <= 'z') ? (char)(c - 32) : (char)c;
}

static char groupLetters[32];
static int groupLetterCount = -1;

static void buildGroups() {
  if (groupLetterCount >= 0) return;
  groupLetterCount = 0;
  for (int i = SNCF_PARIS_COUNT; i < PICK_COUNT; i++) {
    const char c = initial(SNCF_PICKS[i].name);
    if (!memchr(groupLetters, c, groupLetterCount) && groupLetterCount < (int)sizeof(groupLetters))
      groupLetters[groupLetterCount++] = c;
  }
}

int sncfGroupCount() {
  buildGroups();
  return 1 + groupLetterCount;
}

const char* sncfGroupLabel(int group) {
  static char buf[2];
  if (group <= 0) return "Paris";
  buildGroups();
  buf[0] = group - 1 < groupLetterCount ? groupLetters[group - 1] : '?';
  buf[1] = 0;
  return buf;
}

int sncfGroupStations(int group, int* out, int maxN) {
  int n = 0;
  if (group <= 0) {
    for (int i = 0; i < SNCF_PARIS_COUNT && n < maxN; i++) out[n++] = i;
    return n;
  }
  buildGroups();
  if (group - 1 >= groupLetterCount) return 0;
  const char c = groupLetters[group - 1];
  for (int i = SNCF_PARIS_COUNT; i < PICK_COUNT && n < maxN; i++)
    if (initial(SNCF_PICKS[i].name) == c) out[n++] = i;
  return n;
}

int sncfPickCount() { return PICK_COUNT; }

const SncfPick& sncfPick(int index) { return SNCF_PICKS[index < 0 || index >= PICK_COUNT ? 0 : index]; }
