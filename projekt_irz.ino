#include "M5Core2.h"
#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
//diody
#include "FastLED.h"
#define LEDS_PIN 25
#define LEDS_NUM 10
CRGB ledsBuff[LEDS_NUM];

//Zmienne pomocne do ustawień zegara i wifi
const char* ssid = "ANIA";
const char* password = "5249555";
const char* ntpServer = "tempus1.gum.gov.pl";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;
char timeStrbuff[64];
char dateStrbuff[64];
int checkWiFiCount = 0, checkWiFi = 0, checkSetupTime = 0;
int isFastLed = 0;
char booleanFastLed[3][4] = { "NIE", "TAK" };
int isSetupFirst = 1, start_setup = 1;
int setup_hours = 0, setup_minutes = 0, setup_day = 1, setup_month = 1, setup_year = 2023;

//Zmienne opowiedzialne za wyświetlanie odpowiednich rzeczy na ekranie
int current_page = 0, current_main_page = 0, current_add_page = 0, current_all_event_page = 0;
int start_display_main = 1, start_display_add = 1, start_display_all_event = 1, start_display_details = 1;
int current_year, current_month, current_data;
int current_events_count = 1;
int count_alarm = 0, prev_count_alarm = 0;

//Zmienne wykorzystywane podczas odczytywania lub zapisywania wartości do pliku
int *dayRead, *monthRead, *yearRead, *startHoursRead, *startMinutesRead, *endHoursRead, *endMinutesRead, *typesRead;
String strRead;
File file;
int stringCount = 0;
int indexString;
int lineRead;
int searchEvent[7];
int *all_event_current_startHoursRead, *all_event_current_startMinutesRead, *all_event_current_endHoursRead, *all_event_current_endMinutesRead, *all_event_current_typesRead;
int *all_event_today_startHoursRead, *all_event_today_startMinutesRead, *all_event_today_endHoursRead, *all_event_today_endMinutesRead, *all_event_today_typesRead;
int all_event_current_day, all_event_today_day = 0;
int next_day, next_month, next_year;

int event_day, event_month, event_year, event_hours_start, event_minutes_start, event_hours_end, event_minutes_end, event_type_number;
char event_type[15][20] = {
  "Posilek",
  "Nauka",
  "Cwiczenia",
  "Spotkanie",
  "Opoczynek",
  "Porzadki",
  "Drzemka",
  "Spacer",
  "Wyjazd",
  "Obowiazki",
  "Rodzina",
  "Znajmomi",
  "Szkolenia",
  "Toaleta",
  "Inne"
};
int deleteError;
int sortedTypesRead, sortedDayRead, sortedMonthRead, sortedYearRead, sortedStartHoursRead, sortedStartMinutesRead, sortedEndHoursRead, sortedEndMinutesRead;

//Ustawienie wirtualnych
Button detailsBtn(110, 160, 150, 20, "detailsBtn");
Button addOrDeleteBtn(110, 200, 100, 30, "addOrDeleteBtn");
Button allEventBtn(120, 100, 80, 20);
Button setupBtn(0, 0, 320, 10);
Button event_add1(210, 90, 40, 35);
Button event_sub1(260, 90, 40, 35);
Button event_add2(210, 140, 40, 35);
Button event_sub2(260, 140, 40, 35);
Button event_add3(210, 190, 40, 35);
Button event_sub3(260, 190, 40, 35);

int isPressedEventAdd1, isPressedEventAdd2, isPressedEventAdd3 = 0;
int isPressedEventSub1, isPressedEventSub2, isPressedEventSub3 = 0;

//Ustawienie Wifi
void setupWifi() {
  delay(10);
  M5.Lcd.printf("Connecting to %s", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  checkWiFiCount = 0;
  while (checkWiFiCount < 10) {
    if (WiFi.status() == WL_CONNECTED) break;
    delay(500);
    M5.Lcd.print(".");
    checkWiFiCount++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    checkWiFi = 1;
    M5.Lcd.printf("\nSuccess\n");
  } else
    M5.Lcd.printf("\nNot connected\n");
}

//Ustawienie czasu
void setupTime(int hours, int minutes, int seconds) {
  RTCtime.Hours = hours;
  RTCtime.Minutes = minutes;
  RTCtime.Seconds = seconds;
  if (!M5.Rtc.SetTime(&RTCtime)) Serial.println("wrong time set!");
}

void setupCurrentData(int year, int month, int data) {
  current_year = year;
  current_month = month;
  current_data = data;
}

void setupData(int year, int month, int data) {
  RTCDate.Year = year;
  RTCDate.Month = month;
  RTCDate.Date = data;
  if (!M5.Rtc.SetDate(&RTCDate)) Serial.println("wrong date set!");
  else setupCurrentData(year, month, data);
}

void setTimeAndData() {
  if (checkWiFi) {
    struct tm timeinfo;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    if (getLocalTime(&timeinfo)) {
      checkSetupTime = -2;
      setup_hours = timeinfo.tm_hour;
      setup_minutes = timeinfo.tm_min;
      setup_year = timeinfo.tm_year + 1900;
      setup_month = timeinfo.tm_mon + 1;
      setup_day = timeinfo.tm_mday;
      setupTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      setupData(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    }
  }
}

void displayTime() {
  M5.Rtc.GetTime(&RTCtime);
  sprintf(timeStrbuff, "%02d:%02d:%02d", RTCtime.Hours, RTCtime.Minutes, RTCtime.Seconds);
  M5.Lcd.setCursor(0, 1);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print(timeStrbuff);
}

void displayDate() {
  if ((current_main_page != 0 && current_page == 0) || current_page != 0) {
    M5.Rtc.GetDate(&RTCDate);
    sprintf(dateStrbuff, "%02d.%02d.%d", RTCDate.Date, RTCDate.Month, RTCDate.Year);
    M5.Lcd.setCursor(200, 1);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print(dateStrbuff);
  }
}

int checkDayInMonth(int day, int month) {
  int n;
  int* tabMonthDays;

  if (day == 30) n = 4;
  else n = 7;

  tabMonthDays = (int*)malloc(n * sizeof(int));

  if (day == 30) {
    tabMonthDays[0] = 4;
    tabMonthDays[1] = 6;
    tabMonthDays[2] = 9;
    tabMonthDays[3] = 11;
  } else {
    tabMonthDays[0] = 1;
    tabMonthDays[1] = 3;
    tabMonthDays[2] = 5;
    tabMonthDays[3] = 7;
    tabMonthDays[4] = 8;
    tabMonthDays[5] = 10;
    tabMonthDays[6] = 12;
  }

  for (int i = 0; i < n; i++) {
    if (tabMonthDays[i] == month) return 1;
  }
  return 0;
}

void increaseMonthAndYear() {
  if (current_month < 12)
    current_month++;
  else {
    current_month = 1;
    current_year++;
  }
}

void increaseDay(int conditional) {
  if (conditional) current_data++;
  else {
    current_data = 1;
    increaseMonthAndYear();
  }
}

void increaseDate() {

  if (checkDayInMonth(31, current_month))
    increaseDay(current_data < 31 ? 1 : 0);

  else if (checkDayInMonth(30, current_month))
    increaseDay(current_data < 30 ? 1 : 0);

  else {
    if (current_year % 4 == 0)
      increaseDay(current_data < 29 ? 1 : 0);
    else
      increaseDay(current_data < 28 ? 1 : 0);
  }
}

void decreaseData() {
  if (current_data == 1) {
    if (current_month == 1) {
      current_data = 31;
      current_month = 12;
      current_year--;
    }

    else if (checkDayInMonth(31, current_month - 1)) {
      current_data = 31;
      current_month--;
    } else if (checkDayInMonth(30, current_month - 1)) {
      current_data = 30;
      current_month--;
    } else {
      if (current_year % 4 == 0) current_data = 29;
      else current_data = 28;
      current_month = 2;
    }
  } else current_data--;
}

void displayRefresh() {
  M5.Lcd.clear();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
}

void findEvent() {
  all_event_current_day = 0;
  all_event_today_day = 0;
  M5.Rtc.GetDate(&RTCDate);
  for (int i = 0; i < lineRead; i++) {
    if (yearRead[i] == current_year && monthRead[i] == current_month && dayRead[i] == current_data) {
      all_event_current_startHoursRead = (int*)realloc(all_event_current_startHoursRead, (all_event_current_day + 1) * sizeof(int));
      all_event_current_startMinutesRead = (int*)realloc(all_event_current_startMinutesRead, (all_event_current_day + 1) * sizeof(int));
      all_event_current_endHoursRead = (int*)realloc(all_event_current_endHoursRead, (all_event_current_day + 1) * sizeof(int));
      all_event_current_endMinutesRead = (int*)realloc(all_event_current_endMinutesRead, (all_event_current_day + 1) * sizeof(int));
      all_event_current_typesRead = (int*)realloc(all_event_current_typesRead, (all_event_current_day + 1) * sizeof(int));

      all_event_current_startHoursRead[all_event_current_day] = startHoursRead[i];
      all_event_current_startMinutesRead[all_event_current_day] = startMinutesRead[i];
      all_event_current_endHoursRead[all_event_current_day] = endHoursRead[i];
      all_event_current_endMinutesRead[all_event_current_day] = endMinutesRead[i];
      all_event_current_typesRead[all_event_current_day] = typesRead[i];
      all_event_current_day++;
    }
    if (yearRead[i] == RTCDate.Year && monthRead[i] == RTCDate.Month && dayRead[i] == RTCDate.Date) {
      all_event_today_startHoursRead = (int*)realloc(all_event_today_startHoursRead, (all_event_today_day + 1) * sizeof(int));
      all_event_today_startMinutesRead = (int*)realloc(all_event_today_startMinutesRead, (all_event_today_day + 1) * sizeof(int));
      all_event_today_endHoursRead = (int*)realloc(all_event_today_endHoursRead, (all_event_today_day + 1) * sizeof(int));
      all_event_today_endMinutesRead = (int*)realloc(all_event_today_endMinutesRead, (all_event_today_day + 1) * sizeof(int));
      all_event_today_typesRead = (int*)realloc(all_event_today_typesRead, (all_event_today_day + 1) * sizeof(int));

      all_event_today_startHoursRead[all_event_today_day] = startHoursRead[i];
      all_event_today_startMinutesRead[all_event_today_day] = startMinutesRead[i];
      all_event_today_endHoursRead[all_event_today_day] = endHoursRead[i];
      all_event_today_endMinutesRead[all_event_today_day] = endMinutesRead[i];
      all_event_today_typesRead[all_event_today_day] = typesRead[i];
      all_event_today_day++;
    }
  }
  Serial.printf("Line Read %d\n", lineRead);
  Serial.printf("Event Current %d\n", all_event_current_day);
}

void displayMainPage() {
  M5.Rtc.GetDate(&RTCDate);
  if (current_year == RTCDate.Year && current_month == RTCDate.Month && current_data == RTCDate.Date) {
    M5.Lcd.setCursor(140, 30);
    M5.Lcd.print("DZIS");
  }
  if (current_events_count == 1)
    findEvent();
  M5.Lcd.setCursor(50, 50);
  M5.Lcd.setTextSize(4);
  M5.Lcd.printf("%02d.%02d.%d", current_data, current_month, current_year);

  if (all_event_current_day != 0) {
    M5.Lcd.setCursor(260, 100);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("%d/%d", current_events_count, all_event_current_day);

    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(10, 125);
    M5.Lcd.printf("%02d:%02d", all_event_current_startHoursRead[current_events_count - 1], all_event_current_startMinutesRead[current_events_count - 1]);

    M5.Lcd.setCursor(10, 160);
    M5.Lcd.printf("%02d:%02d", all_event_current_endHoursRead[current_events_count - 1], all_event_current_endMinutesRead[current_events_count - 1]);

    for (int i = 0; i < all_event_current_day; i++) {
      if (current_events_count - 1 != i) {
        if ((all_event_current_startHoursRead[current_events_count - 1] == all_event_current_startHoursRead[i] && all_event_current_startMinutesRead[current_events_count - 1] == all_event_current_startMinutesRead[i])

            //czas rozpoczecia1<czas rozpoczecia2 && czas zakonczenia1>czas zakonczenia2  && czas start1<czas zakonczenia2
            || !(((all_event_current_startHoursRead[current_events_count - 1] < all_event_current_startHoursRead[i]
                   || (all_event_current_startHoursRead[current_events_count - 1] == all_event_current_startHoursRead[i]) && all_event_current_startMinutesRead[current_events_count - 1] < all_event_current_startMinutesRead[i])
                  && (all_event_current_endHoursRead[current_events_count - 1] < all_event_current_endHoursRead[i]
                      || (all_event_current_endHoursRead[current_events_count - 1] == all_event_current_endHoursRead[i]) && all_event_current_endMinutesRead[current_events_count - 1] < all_event_current_endMinutesRead[i])
                  && (all_event_current_endHoursRead[current_events_count - 1] < all_event_current_startHoursRead[i]
                      || (all_event_current_endHoursRead[current_events_count - 1] == all_event_current_startHoursRead[i]) && all_event_current_endMinutesRead[current_events_count - 1] <= all_event_current_startMinutesRead[i]))

                 //czas rozpoczecia1>czas rozpoczecia2 && czas zakonczenia1>czas zakonczenia2 && czas start1>czas zakonczenia2
                 || ((all_event_current_startHoursRead[current_events_count - 1] > all_event_current_startHoursRead[i]
                      || (all_event_current_startHoursRead[current_events_count - 1] == all_event_current_startHoursRead[i]) && all_event_current_startMinutesRead[current_events_count - 1] > all_event_current_startMinutesRead[i])
                     && (all_event_current_endHoursRead[current_events_count - 1] > all_event_current_endHoursRead[i]
                         || (all_event_current_endHoursRead[current_events_count - 1] == all_event_current_endHoursRead[i]) && all_event_current_endMinutesRead[current_events_count - 1] > all_event_current_endMinutesRead[i])
                     && (all_event_current_startHoursRead[current_events_count - 1] > all_event_current_endHoursRead[i]
                         || (all_event_current_startHoursRead[current_events_count - 1] == all_event_current_endHoursRead[i]) && all_event_current_startMinutesRead[current_events_count - 1] >= all_event_current_endMinutesRead[i])))) {
          M5.Lcd.setCursor(110, 125);
          M5.Lcd.printf("%s*", event_type[all_event_current_typesRead[current_events_count - 1]]);
          break;
        }
      }
      if (i == all_event_current_day - 1) {
        M5.Lcd.setCursor(110, 125);
        M5.Lcd.print(event_type[all_event_current_typesRead[current_events_count - 1]]);
        break;
      }
    }

    M5.Lcd.setTextSize(2);
    M5.Lcd.fillRect(120, 100, 80, 20, BLUE);
    M5.Lcd.setTextColor(WHITE, BLUE);
    M5.Lcd.setCursor(125, 103);
    M5.Lcd.print("Wiecej");

    M5.Rtc.GetTime(&RTCtime);
    M5.Rtc.GetDate(&RTCDate);
    if ((current_data == RTCDate.Date && current_month == RTCDate.Month && current_year == RTCDate.Year) && (all_event_current_startHoursRead[current_events_count - 1] < RTCtime.Hours || (all_event_current_startHoursRead[current_events_count - 1] == RTCtime.Hours && all_event_current_startMinutesRead[current_events_count - 1] <= RTCtime.Minutes))
        && (all_event_current_endHoursRead[current_events_count - 1] > RTCtime.Hours || (all_event_current_endHoursRead[current_events_count - 1] == RTCtime.Hours && all_event_current_endMinutesRead[current_events_count - 1] > RTCtime.Minutes))) {
      M5.Lcd.fillRect(110, 160, 150, 20, GREEN);
      M5.Lcd.setTextColor(BLACK, GREEN);
      M5.Lcd.setCursor(160, 162);
      M5.Lcd.print("TRWA");

    } else {
      M5.Lcd.fillRect(110, 160, 150, 20, RED);
      M5.Lcd.setTextColor(WHITE, RED);
      M5.Lcd.setCursor(132, 162);
      M5.Lcd.print("Szczegoly");
    }

  } else {
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(50, 132);
    M5.Lcd.print("Brak wydarzen");
  }
  M5.Lcd.fillRect(110, 200, 100, 30, BLUE);
  M5.Lcd.setTextColor(WHITE, BLUE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(155, 210);
  M5.Lcd.print("+");

  M5.Lcd.setTextColor(WHITE, BLACK);
}

void displayAddPage() {
  M5.Lcd.setCursor(110, 40);
  M5.Lcd.print("DODAWANIE");

  switch (current_add_page) {
    case 0:
      M5.Lcd.setCursor(260, 40);
      M5.Lcd.print("1/4");
      M5.Lcd.setCursor(140, 60);
      M5.Lcd.print("Data");

      M5.Lcd.setCursor(30, 100);
      M5.Lcd.print("Dzien");
      M5.Lcd.drawRect(130, 90, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 100);
      M5.Lcd.printf("%02d", event_day);
      M5.Lcd.fillRect(210, 90, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 100);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 90, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 100);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(30, 150);
      M5.Lcd.print("Miesiac");
      M5.Lcd.drawRect(130, 140, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 150);
      M5.Lcd.printf("%02d", event_month);
      M5.Lcd.fillRect(210, 140, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 150);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 140, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 150);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(30, 200);
      M5.Lcd.print("Rok");
      M5.Lcd.drawRect(130, 190, 70, 35, WHITE);
      M5.Lcd.setCursor(140, 200);
      M5.Lcd.printf("%02d", event_year);
      M5.Lcd.fillRect(210, 190, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 200);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 190, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 200);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);
      break;
    case 1:
      M5.Lcd.setCursor(260, 40);
      M5.Lcd.print("2/4");
      M5.Lcd.setCursor(70, 60);
      M5.Lcd.print("Czas rozpoczecia");

      M5.Lcd.setCursor(30, 100);
      M5.Lcd.print("Godzina");
      M5.Lcd.drawRect(130, 90, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 100);
      M5.Lcd.printf("%02d", event_hours_start);
      M5.Lcd.fillRect(210, 90, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 100);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 90, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 100);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(30, 150);
      M5.Lcd.print("Minuta");
      M5.Lcd.drawRect(130, 140, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 150);
      M5.Lcd.printf("%02d", event_minutes_start);
      M5.Lcd.fillRect(210, 140, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 150);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 140, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 150);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(50, 200);
      M5.Lcd.print("Data");
      M5.Lcd.setCursor(150, 200);
      M5.Lcd.printf("%02d.%02d.%04d", event_day, event_month, event_year);

      break;
    case 2:
      M5.Lcd.setCursor(260, 40);
      M5.Lcd.print("3/4");
      M5.Lcd.setCursor(70, 60);
      M5.Lcd.print("Czas zakonczenia");

      M5.Lcd.setCursor(30, 100);
      M5.Lcd.print("Godzina");
      M5.Lcd.drawRect(130, 90, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 100);
      M5.Lcd.printf("%02d", event_hours_end);
      M5.Lcd.fillRect(210, 90, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 100);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 90, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 100);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(30, 150);
      M5.Lcd.print("Minuta");
      M5.Lcd.drawRect(130, 140, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 150);
      M5.Lcd.printf("%02d", event_minutes_end);
      M5.Lcd.fillRect(210, 140, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 150);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 140, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 150);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(50, 190);
      M5.Lcd.print("Data");
      M5.Lcd.setCursor(150, 190);
      M5.Lcd.printf("%02d.%02d.%04d", event_day, event_month, event_year);

      M5.Lcd.setCursor(50, 220);
      M5.Lcd.print("Start");
      M5.Lcd.setCursor(180, 220);
      M5.Lcd.printf("%02d:%02d", event_hours_start, event_minutes_start);

      break;
    case 3:
      M5.Lcd.setCursor(260, 40);
      M5.Lcd.print("4/4");
      M5.Lcd.setCursor(70, 60);
      M5.Lcd.print("Rodzaj wydarzenia");

      M5.Lcd.drawRect(30, 90, 170, 35, WHITE);
      M5.Lcd.setCursor(40, 100);
      M5.Lcd.print(event_type[event_type_number]);
      M5.Lcd.fillRect(210, 90, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(223, 100);
      M5.Lcd.print("<");
      M5.Lcd.fillRect(260, 90, 40, 35, BLUE);
      M5.Lcd.setCursor(273, 100);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.print(">");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(50, 130);
      M5.Lcd.print("Data");
      M5.Lcd.setCursor(150, 130);
      M5.Lcd.printf("%02d.%02d.%04d", event_day, event_month, event_year);

      M5.Lcd.setCursor(50, 150);
      M5.Lcd.print("Start");
      M5.Lcd.setCursor(180, 150);
      M5.Lcd.printf("%02d:%02d", event_hours_start, event_minutes_start);

      M5.Lcd.setCursor(50, 170);
      M5.Lcd.print("Koniec");
      M5.Lcd.setCursor(180, 170);
      M5.Lcd.printf("%02d:%02d", event_hours_end, event_minutes_end);
      if ((event_hours_end == event_hours_start && event_minutes_end == event_minutes_start) || event_hours_end < event_hours_start || (event_hours_end == event_hours_start && event_minutes_end < event_minutes_start)) {
        M5.Lcd.printf("*");
      }
      M5.Lcd.fillRect(110, 200, 100, 30, BLUE);
      M5.Lcd.setCursor(132, 208);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.print("Dodaj");
      M5.Lcd.setTextColor(WHITE, BLACK);

      break;
  }
}

void displayAllEvent() {
  M5.Lcd.setCursor(10, 22);
  int all_page = 0;
  if (all_event_current_day % 9 == 0) {
    all_page = int(all_event_current_day / 9);
  } else {
    all_page = int(all_event_current_day / 9) + 1;
  }

  M5.Lcd.printf("WYDARZENIA %02d.%02d.%04d %d/%d", current_data, current_month, current_year, 1 + current_all_event_page, all_page);
  int count = 0;
  int numberArray = 0;
  for (int i = 0; i < 9; i++) {
    if (i + current_all_event_page * 9 == all_event_current_day) break;
    M5.Lcd.setCursor(20, 45 + i * 20);
    count = 0;
    numberArray = 0;
    for (int j = 0; j < all_event_current_day; j++) {
      if ((i + current_all_event_page * 9) != j) {
        if ((all_event_current_startHoursRead[i + current_all_event_page * 9] == all_event_current_startHoursRead[j] && all_event_current_startMinutesRead[i + current_all_event_page * 9] == all_event_current_startMinutesRead[j])
            //czas rozpoczecia1 <czas rozpoczecia2 && czas zakonczenia1<czas zakonczenia2 && czaszakonczenia1<czas_ropoczecia2
            || !(((all_event_current_startHoursRead[i + current_all_event_page * 9] < all_event_current_startHoursRead[j]
                   || (all_event_current_startHoursRead[i + current_all_event_page * 9] == all_event_current_startHoursRead[j]) && all_event_current_startMinutesRead[i + current_all_event_page * 9] < all_event_current_startMinutesRead[j])
                  && (all_event_current_endHoursRead[i + current_all_event_page * 9] < all_event_current_endHoursRead[j]
                      || (all_event_current_endHoursRead[i + current_all_event_page * 9] == all_event_current_endHoursRead[j]) && all_event_current_endMinutesRead[i + current_all_event_page * 9] < all_event_current_endMinutesRead[j])
                  && (all_event_current_endHoursRead[i + current_all_event_page * 9] < all_event_current_startHoursRead[j]
                      || (all_event_current_endHoursRead[i + current_all_event_page * 9] == all_event_current_startHoursRead[j]) && all_event_current_endMinutesRead[i + current_all_event_page * 9] <= all_event_current_startMinutesRead[j]))

                 //czas rozpoczecia1>czas rozpoczecia2 && czas zakonczenia1>czas zakonczenia2 && czasrozpoczecia1>czas_zakonczeniaa2
                 || ((all_event_current_startHoursRead[i + current_all_event_page * 9] > all_event_current_startHoursRead[j]
                      || (all_event_current_startHoursRead[i + current_all_event_page * 9] == all_event_current_startHoursRead[j]) && all_event_current_startMinutesRead[i + current_all_event_page * 9] > all_event_current_startMinutesRead[j])
                     && (all_event_current_endHoursRead[i + current_all_event_page * 9] > all_event_current_endHoursRead[j]
                         || (all_event_current_endHoursRead[i + current_all_event_page * 9] == all_event_current_endHoursRead[j]) && all_event_current_endMinutesRead[i + current_all_event_page * 9] > all_event_current_endMinutesRead[j])
                     && (all_event_current_startHoursRead[i + current_all_event_page * 9] > all_event_current_endHoursRead[j]
                         || (all_event_current_startHoursRead[i + current_all_event_page * 9] == all_event_current_endHoursRead[j]) && all_event_current_startMinutesRead[i + current_all_event_page * 9] >= all_event_current_endMinutesRead[j]))))

        {
          if (count == 0) {
            if (j < (i + current_all_event_page * 9)) {
              numberArray = j;
            } else {
              numberArray = i + current_all_event_page * 9;
            }
          }
          count++;
        }
      }
    }
    M5.Lcd.printf("%02d:%02d-%02d:%02d %s",
                  all_event_current_startHoursRead[i + current_all_event_page * 9], all_event_current_startMinutesRead[i + current_all_event_page * 9],
                  all_event_current_endHoursRead[i + current_all_event_page * 9], all_event_current_endMinutesRead[i + current_all_event_page * 9],
                  event_type[all_event_current_typesRead[i + current_all_event_page * 9]]);
    if (count != 0) {
      if (numberArray % 4 == 0)
        M5.Lcd.setTextColor(RED, BLACK);
      else if (numberArray % 4 == 1)
        M5.Lcd.setTextColor(GREEN, BLACK);
      else if (numberArray % 4 == 2)
        M5.Lcd.setTextColor(BLUE, BLACK);
      else if (numberArray % 4 == 3)
        M5.Lcd.setTextColor(YELLOW, BLACK);

      M5.Lcd.printf("%d*", count);
      M5.Lcd.setTextColor(WHITE, BLACK);
    }
  }
}

void displayDetailsPage() {
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 35);
  M5.Lcd.print("Data");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(130, 30);
  M5.Lcd.printf("%02d.%02d.%04d", current_data, current_month, current_year);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 80);
  M5.Lcd.print("Start");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(130, 75);
  M5.Lcd.printf("%02d:%02d", all_event_current_startHoursRead[current_events_count - 1], all_event_current_startMinutesRead[current_events_count - 1]);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 125);
  M5.Lcd.print("Koniec");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(130, 120);
  M5.Lcd.printf("%02d:%02d", all_event_current_endHoursRead[current_events_count - 1], all_event_current_endMinutesRead[current_events_count - 1]);

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 170);
  M5.Lcd.print("Rodzaj");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(130, 165);
  M5.Lcd.print(event_type[all_event_current_typesRead[current_events_count - 1]]);


  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLUE);
  M5.Lcd.fillRect(110, 200, 100, 30, BLUE);
  M5.Lcd.setCursor(140, 210);
  M5.Lcd.print("Usun");

  M5.Lcd.setTextColor(WHITE, BLACK);
}

void setupParametres() {
  current_main_page = 0;
  current_add_page = 0;
  current_all_event_page = 0;
  current_events_count = 1;

  start_display_main = 1;
  start_display_add = 1;
  start_display_all_event = 1;
  start_display_details = 1;
}

void handleDetails(Event& e) {
  if (current_page == 0 && checkSetupTime > 0) {
    current_page = 2;
    start_display_main = 1;
    start_display_details = 1;
    displayRefresh();
  }
}

void displayBannerAdd() {
  displayRefresh();
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setCursor(120, 110);
  M5.Lcd.print("DODANO");
  delay(2000);
  M5.Rtc.GetDate(&RTCDate);
  setupCurrentData(RTCDate.Year, RTCDate.Month, RTCDate.Date);
  delay(2000);
  displayRefresh();
}

void displayBannerError() {
  displayRefresh();
  M5.Lcd.setCursor(5, 110);
  M5.Lcd.setTextColor(RED, BLACK);
  M5.Lcd.print("BLAD. SPROBOJ JESZCZE RAZ!");
  delay(2000);
  displayRefresh();
}

void handleAddOrDelete(Event& e) {
  if (checkSetupTime == -2) {
    setupTime(setup_hours, setup_minutes, 0);
    setupData(setup_year, setup_month, setup_day);
    setupParametres();
    current_page = 0;
    displayRefresh();
    checkSetupTime = 1;
  } else if (current_page == 0) {
    current_page = 1;
    start_display_main = 1;
    start_display_add = 1;

    event_day = current_data;
    event_month = current_month;
    event_year = current_year;
    event_hours_start = 12;
    event_minutes_start = 0;
    event_hours_end = 12;
    event_minutes_end = 0;
    event_type_number = 0;
    displayRefresh();
  }
  if (current_page == 1) {
    if (current_add_page == 3) {
      displayRefresh();
      M5.Lcd.setCursor(110, 110);
      M5.Lcd.print("DODAWANIE");
      delay(1000);
      File file = SD.open("/events.txt", FILE_APPEND);
      if (!file)
        file = SD.open("/events.txt", FILE_WRITE);
      if ((event_hours_end == event_hours_start && event_minutes_end == event_minutes_start) || event_hours_end < event_hours_start || (event_hours_end == event_hours_start && event_minutes_end < event_minutes_start)) {
        next_day = event_day;
        next_month = event_month;
        next_year = event_year;
        if (!checkDayInMonth(31, event_month)) {
          if (checkDayInMonth(30, event_month)) {
            if (event_day == 30) {
              next_day = 1;
              next_month = event_month + 1;
            } else next_day = event_day + 1;
          } else {
            if (event_year % 4 == 0 && event_day == 29) {
              next_day = 1;
              next_month = 3;
            } else if (event_year % 4 != 0 && event_day == 28) {
              next_day = 1;
              next_month = 3;
            } else next_day = event_day + 1;
          }
        } else {
          if (event_day == 31) {
            next_day = 1;
            next_month = event_month + 1;
            if (event_month == 12) {
              next_month = 1;
              next_year = event_year + 1;
            }
          } else next_day = event_day + 1;
        }
        if (file.printf("%d;%d;%d;%d;%d;%d;%d;%d\n",
                        event_day, event_month, event_year, event_hours_start, event_minutes_start, 23, 59, event_type_number)
            && file.printf("%d;%d;%d;%d;%d;%d;%d;%d\n",
                           next_day, next_month, next_year, 0, 0, event_hours_end, event_minutes_end, event_type_number)) {
          displayBannerAdd();
        } else {
          displayBannerError();
        }
      } else if (file.printf("%d;%d;%d;%d;%d;%d;%d;%d\n",
                             event_day, event_month, event_year, event_hours_start, event_minutes_start, event_hours_end, event_minutes_end, event_type_number)) {
        displayBannerAdd();
      } else {
        displayBannerError();
      }
      file.close();
      setupParametres();
      current_page = 0;
      displayRefresh();
    }
  } else if (current_page == 2) {
    displayRefresh();
    M5.Lcd.setCursor(110, 110);
    M5.Lcd.print("USUWANIE");
    delay(1000);
    File file1 = SD.open("/events.txt");
    File file2 = SD.open("/events_delete.txt", FILE_WRITE);
    while (file1.available()) {
      strRead = file1.readStringUntil('\n');
      stringCount = 0;
      deleteError = 0;
      while (strRead.length() > 0) {
        indexString = strRead.indexOf(";");
        if (indexString == -1) {
          searchEvent[stringCount] = atoi(strRead.c_str());
          if (!(searchEvent[0] == current_data && searchEvent[1] == current_month
                && searchEvent[2] == current_year
                && searchEvent[3] == all_event_current_startHoursRead[current_events_count - 1]
                && searchEvent[4] == all_event_current_startMinutesRead[current_events_count - 1]
                && searchEvent[5] == all_event_current_endHoursRead[current_events_count - 1]
                && searchEvent[6] == all_event_current_endMinutesRead[current_events_count - 1]
                && searchEvent[7] == all_event_current_typesRead[current_events_count - 1])) {
            if (!file2.printf("%d;%d;%d;%d;%d;%d;%d;%d\n",
                              searchEvent[0], searchEvent[1], searchEvent[2], searchEvent[3], searchEvent[4], searchEvent[5], searchEvent[6], searchEvent[7])) {
              displayRefresh();
              M5.Lcd.setCursor(5, 110);
              M5.Lcd.setTextColor(RED, BLACK);
              M5.Lcd.print("BLAD. SPROBOJ JESZCZE RAZ!");
              delay(2000);
              displayRefresh();
              deleteError = 1;
            }
          }
          break;
        } else {
          searchEvent[stringCount] = atoi(strRead.substring(0, indexString).c_str());
          stringCount++;
          strRead = strRead.substring(indexString + 1);
        }
      }
    }
    file1.close();
    file2.close();
    Serial.print("USUWANIE");
    if (deleteError == 0) {
      File file1 = SD.open("/events.txt", FILE_WRITE);
      File file2 = SD.open("/events_delete.txt");
      deleteError = 1;
      Serial.print("USUWANIE");
      while (file2.available()) {
        strRead = file2.readStringUntil('\n');
        Serial.print("USUWANIE");
        if (!file1.println(strRead)) {
          displayRefresh();
          M5.Lcd.setCursor(5, 110);
          M5.Lcd.setTextColor(RED, BLACK);
          M5.Lcd.print("BLAD. SPROBOJ JESZCZE RAZ!");
          delay(2000);
          displayRefresh();
          deleteError = 1;
        }
        deleteError = 0;
      }
      file1.close();
      file2.close();
    }
    if (deleteError) {
      displayRefresh();
      M5.Lcd.setCursor(5, 110);
      M5.Lcd.setTextColor(RED, BLACK);
      M5.Lcd.print("BLAD. SPROBOJ JESZCZE RAZ!");
      delay(2000);
      displayRefresh();
      deleteError = 1;
    } else {
      SD.remove("/events_delete.txt");
      displayRefresh();
      M5.Lcd.setTextColor(GREEN, BLACK);
      M5.Lcd.setCursor(105, 110);
      M5.Lcd.print("USUNIETO");
      delay(2000);
      displayRefresh();
    }
    current_page = 0;
    start_display_details = 1;
    start_display_main = 1;
    setupParametres();
    M5.Rtc.GetDate(&RTCDate);
    setupData(RTCDate.Year, RTCDate.Month, RTCDate.Date);
    displayRefresh();
  }
}


void readEvent() {
  file = SD.open("/events.txt");
  lineRead = 0;
  while (file.available()) {
    strRead = file.readStringUntil('\n');

    stringCount = 0;

    dayRead = (int*)realloc(dayRead, (lineRead + 1) * sizeof(int));
    monthRead = (int*)realloc(monthRead, (lineRead + 1) * sizeof(int));
    yearRead = (int*)realloc(yearRead, (lineRead + 1) * sizeof(int));
    startHoursRead = (int*)realloc(startHoursRead, (lineRead + 1) * sizeof(int));
    startMinutesRead = (int*)realloc(startMinutesRead, (lineRead + 1) * sizeof(int));
    endHoursRead = (int*)realloc(endHoursRead, (lineRead + 1) * sizeof(int));
    endMinutesRead = (int*)realloc(endMinutesRead, (lineRead + 1) * sizeof(int));
    typesRead = (int*)realloc(typesRead, (lineRead + 1) * sizeof(int));

    while (strRead.length() > 0) {
      indexString = strRead.indexOf(";");
      if (indexString == -1) {
        typesRead[lineRead] = atoi(strRead.c_str());
        break;
      } else {
        if (stringCount == 0) dayRead[lineRead] = atoi(strRead.substring(0, indexString).c_str());
        else if (stringCount == 1) monthRead[lineRead] = atoi(strRead.substring(0, indexString).c_str());
        else if (stringCount == 2) yearRead[lineRead] = atoi(strRead.substring(0, indexString).c_str());
        else if (stringCount == 3) startHoursRead[lineRead] = atoi(strRead.substring(0, indexString).c_str());
        else if (stringCount == 4) startMinutesRead[lineRead] = atoi(strRead.substring(0, indexString).c_str());
        else if (stringCount == 5) endHoursRead[lineRead] = atoi(strRead.substring(0, indexString).c_str());
        else if (stringCount == 6) endMinutesRead[lineRead] = atoi(strRead.substring(0, indexString).c_str());

        stringCount++;
        strRead = strRead.substring(indexString + 1);
      }
    }
    lineRead++;
  }
  file.close();
  for (int i = 0; i < lineRead; i++) {
    for (int j = i + 1; j < lineRead; j++) {
      if (startHoursRead[i] > startHoursRead[j] || (startHoursRead[i] == startHoursRead[j] && startMinutesRead[i] > startMinutesRead[j])
          || (startHoursRead[i] == startHoursRead[j] && startMinutesRead[i] == startMinutesRead[j] && endHoursRead[i] > endHoursRead[j])
          || (startHoursRead[i] == startHoursRead[j] && startMinutesRead[i] == startMinutesRead[j] && endHoursRead[i] == endHoursRead[j] && endMinutesRead[i] > endMinutesRead[j])) {
        sortedTypesRead = typesRead[i];
        sortedDayRead = dayRead[i];
        sortedMonthRead = monthRead[i];
        sortedYearRead = yearRead[i];
        sortedStartHoursRead = startHoursRead[i];
        sortedStartMinutesRead = startMinutesRead[i];
        sortedEndHoursRead = endHoursRead[i];
        sortedEndMinutesRead = endMinutesRead[i];

        typesRead[i] = typesRead[j];
        dayRead[i] = dayRead[j];
        monthRead[i] = monthRead[j];
        yearRead[i] = yearRead[j];
        startHoursRead[i] = startHoursRead[j];
        startMinutesRead[i] = startMinutesRead[j];
        endHoursRead[i] = endHoursRead[j];
        endMinutesRead[i] = endMinutesRead[j];

        typesRead[j] = sortedTypesRead;
        dayRead[j] = sortedDayRead;
        monthRead[j] = sortedMonthRead;
        yearRead[j] = sortedYearRead;
        startHoursRead[j] = sortedStartHoursRead;
        startMinutesRead[j] = sortedStartMinutesRead;
        endHoursRead[j] = sortedEndHoursRead;
        endMinutesRead[j] = sortedEndMinutesRead;
      }
    }
  }
}

void handleEventAdd1(Event& e) {
  Button& b = *e.button;
  isPressedEventAdd1 = b.isPressed();
}

void actionEventAdd1() {
  if (isPressedEventAdd1) {
    if (checkSetupTime == 0) {
      if (setup_day < 31) {
        setup_day++;
        displayRefresh();
        start_setup = 1;
      } else {
        setup_day = 1;
        displayRefresh();
        start_setup = 1;
      }
    } else if (checkSetupTime == -1) {
      if (setup_hours < 23) {
        setup_hours++;
        displayRefresh();
        start_setup = 1;
      } else {
        setup_hours = 0;
        displayRefresh();
        start_setup = 1;
      }
    } else if (checkSetupTime == -2) {
      if (isFastLed == 1)
        isFastLed = 0;
      else isFastLed = 1;
      displayRefresh();
      start_setup = 1;
    } else if (current_page == 1) {
      if (current_add_page == 0) {
        if (event_day < 31) {
          event_day++;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_day = 1;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 1) {
        if (event_hours_start < 23) {
          event_hours_start++;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_hours_start = 0;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 2) {
        if (event_hours_end < 23) {
          event_hours_end++;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_hours_end = 0;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 3) {
        if (event_type_number != 0) {
          event_type_number--;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_type_number = 14;
          displayRefresh();
          start_display_add = 1;
        }
      }
    }
  }
}

void handleEventSub1(Event& e) {
  Button& b = *e.button;
  isPressedEventSub1 = b.isPressed();
}

void actionEventSub1() {
  if (isPressedEventSub1) {
    if (checkSetupTime == 0) {
      if (setup_day != 1) {
        setup_day--;
        displayRefresh();
        start_setup = 1;
      } else {
        setup_day = 31;
        displayRefresh();
        start_setup = 1;
      }
    } else if (checkSetupTime == -1) {
      if (setup_hours != 0) {
        setup_hours--;
        displayRefresh();
        start_setup = 1;
      } else {
        setup_hours = 23;
        displayRefresh();
        start_setup = 1;
      }
    } else if (checkSetupTime == -2) {
      if (isFastLed == 1)
        isFastLed = 0;
      else isFastLed = 1;
      displayRefresh();
      start_setup = 1;
    } else if (current_page == 1) {
      if (current_add_page == 0) {
        if (event_day != 1) {
          event_day--;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_day = 31;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 1) {
        if (event_hours_start != 0) {
          event_hours_start--;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_hours_start = 23;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 2) {
        if (event_hours_end != 0) {
          event_hours_end--;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_hours_end = 23;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 3) {
        if (event_type_number < 14) {
          event_type_number++;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_type_number = 0;
          displayRefresh();
          start_display_add = 1;
        }
      }
    }
  }
}

void handleEventAdd2(Event& e) {
  Button& b = *e.button;
  isPressedEventAdd2 = b.isPressed();
}

void actionEventAdd2() {
  if (isPressedEventAdd2) {
    if (checkSetupTime == 0) {
      if (setup_month < 12) {
        setup_month++;
        displayRefresh();
        start_setup = 1;
      } else {
        setup_month = 1;
        displayRefresh();
        start_setup = 1;
      }
    } else if (checkSetupTime == -1) {
      if (setup_minutes < 59) {
        setup_minutes++;
        displayRefresh();
        start_setup = 1;
      } else {
        setup_minutes = 0;
        displayRefresh();
        start_setup = 1;
      }
    } else if (current_page == 1) {
      if (current_add_page == 0) {
        if (event_month < 12) {
          event_month++;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_month = 1;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 1) {
        if (event_minutes_start < 59) {
          event_minutes_start++;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_minutes_start = 0;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 2) {
        if (event_minutes_end < 59) {
          event_minutes_end++;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_minutes_end = 0;
          displayRefresh();
          start_display_add = 1;
        }
      }
    }
  }
}

void handleEventSub2(Event& e) {
  Button& b = *e.button;
  isPressedEventSub2 = b.isPressed();
}

void actionEventSub2() {
  if (isPressedEventSub2) {
    if (checkSetupTime == 0) {
      if (setup_month != 1) {
        setup_month--;
        displayRefresh();
        start_setup = 1;
      } else {
        setup_month = 12;
        displayRefresh();
        start_setup = 1;
      }
    } else if (checkSetupTime == -1) {
      if (setup_minutes != 0) {
        setup_minutes--;
        displayRefresh();
        start_setup = 1;
      } else {
        setup_minutes = 59;
        displayRefresh();
        start_setup = 1;
      }
    } else if (current_page == 1) {
      if (current_add_page == 0) {
        if (event_month != 1) {
          event_month--;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_month = 12;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 1) {
        if (event_minutes_start != 0) {
          event_minutes_start--;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_minutes_start = 59;
          displayRefresh();
          start_display_add = 1;
        }
      } else if (current_add_page == 2) {
        if (event_minutes_end != 0) {
          event_minutes_end--;
          displayRefresh();
          start_display_add = 1;
        } else {
          event_minutes_end = 59;
          displayRefresh();
          start_display_add = 1;
        }
      }
    }
  }
}

void handleEventAdd3(Event& e) {
  Button& b = *e.button;
  isPressedEventAdd3 = b.isPressed();
}

void actionEventAdd3() {
  if (isPressedEventAdd3) {
    if (checkSetupTime == 0) {
      setup_year++;
      displayRefresh();
      start_setup = 1;
    } else if (current_page == 1) {
      if (current_add_page == 0) {
        event_year++;
        displayRefresh();
        start_display_add = 1;
      }
    }
  }
}

void handleEventSub3(Event& e) {
  Button& b = *e.button;
  isPressedEventSub3 = b.isPressed();
}

void actionEventSub3() {
  if (isPressedEventSub3) {
    if (checkSetupTime == 0) {
      if (setup_year > 2023) {
        setup_year--;
        displayRefresh();
        start_setup = 1;
      }
    } else if (current_page == 1) {
      M5.Rtc.GetDate(&RTCDate);
      if (current_add_page == 0 && event_year > RTCDate.Year) {
        event_year--;
        displayRefresh();
        start_display_add = 1;
      }
    }
  }
}

void displaySetupDateAndTime() {
  switch (checkSetupTime) {
    case 0:
      M5.Lcd.setTextSize(2);
      if (checkWiFi == 0) {
        M5.Lcd.setCursor(20, 30);
        M5.Lcd.print("BRAK POLACZENIA Z WI-FI");
      } else {
        M5.Lcd.setCursor(100, 30);
        M5.Lcd.print("USTAWIENIA");
      }
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(110, 60);
      M5.Lcd.print("Ustaw date");

      M5.Lcd.setCursor(260, 60);
      M5.Lcd.print("1/3");
      M5.Lcd.setCursor(30, 100);
      M5.Lcd.print("Dzien");
      M5.Lcd.drawRect(130, 90, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 100);
      M5.Lcd.printf("%02d", setup_day);
      M5.Lcd.fillRect(210, 90, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 100);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 90, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 100);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(30, 150);
      M5.Lcd.print("Miesiac");
      M5.Lcd.drawRect(130, 140, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 150);
      M5.Lcd.printf("%02d", setup_month);
      M5.Lcd.fillRect(210, 140, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 150);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 140, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 150);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(30, 200);
      M5.Lcd.print("Rok");
      M5.Lcd.drawRect(130, 190, 70, 35, WHITE);
      M5.Lcd.setCursor(140, 200);
      M5.Lcd.printf("%02d", setup_year);
      M5.Lcd.fillRect(210, 190, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 200);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 190, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 200);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);
      break;
    case -1:
      M5.Lcd.setTextSize(2);
      if (checkWiFi == 0) {
        M5.Lcd.setCursor(20, 30);
        M5.Lcd.print("BRAK POLACZENIA Z WI-FI");
      } else {
        M5.Lcd.setCursor(100, 30);
        M5.Lcd.print("USTAWIENIA");
      }
      M5.Lcd.setCursor(260, 60);
      M5.Lcd.print("2/3");
      M5.Lcd.setCursor(145, 60);
      M5.Lcd.print("Czas");

      M5.Lcd.setCursor(30, 100);
      M5.Lcd.print("Godzina");
      M5.Lcd.drawRect(130, 90, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 100);
      M5.Lcd.printf("%02d", setup_hours);
      M5.Lcd.fillRect(210, 90, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 100);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 90, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 100);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setCursor(30, 150);
      M5.Lcd.print("Minuta");
      M5.Lcd.drawRect(130, 140, 70, 35, WHITE);
      M5.Lcd.setCursor(155, 150);
      M5.Lcd.printf("%02d", setup_minutes);
      M5.Lcd.fillRect(210, 140, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 150);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 140, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 150);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(50, 198);
      M5.Lcd.print("Data");
      M5.Lcd.setCursor(150, 198);
      M5.Lcd.printf("%02d.%02d.%04d", setup_day, setup_month, setup_year);
      break;
    case -2:  //Diody
      if (checkWiFi == 0) {
        M5.Lcd.setCursor(20, 30);
        M5.Lcd.print("BRAK POLACZENIA Z WI-FI");
      } else {
        M5.Lcd.setCursor(100, 30);
        M5.Lcd.print("USTAWIENIA");
      }

      M5.Lcd.setCursor(260, 60);
      M5.Lcd.print("3/3");
      M5.Lcd.setCursor(140, 60);
      M5.Lcd.print("Diody");

      M5.Lcd.setCursor(30, 100);
      M5.Lcd.print("Swieci");
      M5.Lcd.drawRect(130, 90, 70, 35, WHITE);
      M5.Lcd.setCursor(148, 100);
      M5.Lcd.printf("%s", booleanFastLed[isFastLed]);
      M5.Lcd.fillRect(210, 90, 40, 35, BLUE);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.setCursor(225, 100);
      M5.Lcd.print("+");
      M5.Lcd.fillRect(260, 90, 40, 35, BLUE);
      M5.Lcd.setCursor(275, 100);
      M5.Lcd.print("-");
      M5.Lcd.setTextColor(WHITE, BLACK);

      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(50, 148);
      M5.Lcd.print("Data");
      M5.Lcd.setCursor(150, 148);
      M5.Lcd.printf("%02d.%02d.%04d", setup_day, setup_month, setup_year);

      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(50, 168);
      M5.Lcd.print("Godzina");
      M5.Lcd.setCursor(150, 168);
      M5.Lcd.printf("%02d:%02d", setup_hours, setup_minutes);

      M5.Lcd.setTextSize(2);
      M5.Lcd.fillRect(110, 200, 100, 30, BLUE);
      M5.Lcd.setCursor(132, 208);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.print("Ustaw");
      M5.Lcd.setTextColor(WHITE, BLACK);
      break;
  }
}

void handleSetup(Event& e) {
  if (checkSetupTime > 0) {
    M5.Rtc.GetTime(&RTCtime);
    M5.Rtc.GetDate(&RTCDate);

    displayRefresh();
    checkSetupTime = 0;
    setup_hours = RTCtime.Hours;
    setup_minutes = RTCtime.Minutes;
    setup_day = RTCDate.Date;
    setup_month = RTCDate.Month;
    setup_year = RTCDate.Year;
    start_setup = 1;
  }
}

void handleAllEvent(Event& e) {
  if (checkSetupTime > 0 && current_page == 0 && all_event_current_day != 0) {
    current_page = 3;
    start_display_main = 1;
    start_display_all_event = 1;
    displayRefresh();
  }
}

void setup() {

  M5.begin();
  setupWifi();
  setTimeAndData();
  delay(1000);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);

  detailsBtn.addHandler(handleDetails, E_TOUCH);
  addOrDeleteBtn.addHandler(handleAddOrDelete, E_TOUCH);

  event_add1.addHandler(handleEventAdd1, E_TOUCH + E_RELEASE);
  event_sub1.addHandler(handleEventSub1, E_TOUCH + E_RELEASE);
  event_add2.addHandler(handleEventAdd2, E_TOUCH + E_RELEASE);
  event_sub2.addHandler(handleEventSub2, E_TOUCH + E_RELEASE);
  event_add3.addHandler(handleEventAdd3, E_TOUCH + E_RELEASE);
  event_sub3.addHandler(handleEventSub3, E_TOUCH + E_RELEASE);

  setupBtn.addHandler(handleSetup, E_TOUCH);
  allEventBtn.addHandler(handleAllEvent, E_TOUCH);

  Serial.begin(115200);
  if (!SD.begin(5)) {
    Serial.println("Card Mount Failed");
  }

  FastLED.addLeds<SK6812, LEDS_PIN>(ledsBuff, LEDS_NUM);
  for (int i = 0; i < LEDS_NUM; i++) { ledsBuff[i].setRGB(0, 0, 0); }
  FastLED.show();
}

void loop() {
  M5.update();
  M5.Rtc.GetTime(&RTCtime);
  M5.Rtc.GetDate(&RTCDate);

  count_alarm = 0;
  for (int i = 0; i < all_event_today_day; i++) {
    if ((all_event_today_startHoursRead[i] < RTCtime.Hours || (all_event_today_startHoursRead[i] == RTCtime.Hours && all_event_today_startMinutesRead[i] <= RTCtime.Minutes))
        && (all_event_today_endHoursRead[i] > RTCtime.Hours || (all_event_today_endHoursRead[i] == RTCtime.Hours && all_event_today_endMinutesRead[i] > RTCtime.Minutes))) {
      count_alarm++;
    }
  }

  if (prev_count_alarm != count_alarm && current_page == 0 && checkSetupTime > 0) {
    M5.Lcd.clear();
    start_display_main = 1;
    start_display_add = 1;
    start_display_all_event = 1;
    start_display_details = 1;
    prev_count_alarm = count_alarm;
  }

  if (count_alarm != 0) {
    if (isFastLed == 1) {
      for (int i = 0; i < LEDS_NUM; i++) {
        ledsBuff[i].setRGB(100, 0, 0);
      }
    } else {
      for (int i = 0; i < LEDS_NUM; i++) {
        ledsBuff[i].setRGB(0, 0, 0);
      }
    }
    M5.Lcd.setCursor(110, 1);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("Alarm %d", count_alarm);
  } else {
    for (int i = 0; i < LEDS_NUM; i++) {
      ledsBuff[i].setRGB(0, 0, 0);
    }
    M5.Lcd.setCursor(110, 1);
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("      ");
  }

  FastLED.show();
  actionEventAdd1();
  actionEventAdd2();
  actionEventAdd3();
  actionEventSub1();
  actionEventSub2();
  actionEventSub3();

  if (checkSetupTime > 0) {
    displayTime();
    displayDate();
    isSetupFirst = 0;

    if (RTCtime.Hours == 23 && RTCtime.Minutes == 59 && RTCtime.Seconds > 58) {
      delay(2000);
      setupParametres();
      setupData(RTCDate.Year, RTCDate.Month, RTCDate.Date);
      findEvent();
      delay(1000);
      current_page = 0;
      displayRefresh();
    }

    switch (current_page) {
      case 0:  //main page
        if (start_display_main) {
          readEvent();
          displayMainPage();
          start_display_main = 0;
        }

        if (M5.BtnA.isPressed() && current_main_page != 0) {
          current_events_count = 1;
          current_main_page--;
          decreaseData();
          displayRefresh();
          displayMainPage();
        }

        else if (M5.BtnC.isPressed()) {
          current_events_count = 1;
          current_main_page++;
          increaseDate();
          displayRefresh();
          displayMainPage();
        } else if (M5.BtnB.isPressed()) {
          if (all_event_current_day != current_events_count)
            current_events_count++;
          else current_events_count = 1;
          displayRefresh();
          displayMainPage();
        }
        break;
      case 1:  //add_events
        if (start_display_add) {
          displayAddPage();
          start_display_add = 0;
        }
        if ((M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))) {
          if (current_add_page == 0) {
            current_page = 0;
            start_display_main = 1;
            displayRefresh();
          } else {
            current_add_page--;
            displayRefresh();
            displayAddPage();
          }
        }
        if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200)) {
          if (current_add_page < 3) {
            current_add_page++;
            if (!checkDayInMonth(31, event_month) && current_add_page == 1) {
              if (checkDayInMonth(30, event_month)) {
                if (event_day == 31) event_day = 30;
              } else {
                if (event_year % 4 == 0 && event_day > 29) event_day = 29;
                else if (event_year % 4 != 0 && event_day > 28) event_day = 28;
              }
            }
            if (current_add_page == 1 && event_year == RTCDate.Year && (event_month < RTCDate.Month || (event_month == RTCDate.Month && event_day < RTCDate.Date))) {
              event_day = RTCDate.Date;
              event_month = RTCDate.Month;
              event_year = RTCDate.Year;
            }
            displayRefresh();
            displayAddPage();
          }
        }
        break;
      case 2:  //details_page
        if (start_display_details) {
          displayDetailsPage();
          start_display_details = 0;
        }
        if ((M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))) {
          M5.Lcd.setCursor(200, 200);
          current_page = 0;
          start_display_main = 1;
          start_display_details = 1;
          displayRefresh();
        }
        break;
      case 3:  //all_event_page
        if (start_display_all_event) {
          displayAllEvent();
          start_display_all_event = 0;
        }
        if ((M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))) {
          if (current_all_event_page == 0) {
            current_page = 0;
            start_display_all_event = 1;
            displayRefresh();
          } else {
            current_all_event_page--;
            displayRefresh();
            displayAllEvent();
          }
        }
        if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200)) {
          if ((current_all_event_page + 1) * 9 < all_event_current_day) {
            current_all_event_page++;
            displayRefresh();
            displayAllEvent();
          }
        }
        break;
    }
  } else {
    if (start_setup) {
      displaySetupDateAndTime();
      start_setup = 0;
    }
    if ((M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200)) && (checkSetupTime == -1 || isSetupFirst == 0)) {
      checkSetupTime++;
      displayRefresh();
      displaySetupDateAndTime();
      if (checkSetupTime == 1) {
        setupParametres();
        setupCurrentData(RTCDate.Year, RTCDate.Month, RTCDate.Date);
        current_page = 0;
        displayRefresh();
      }
    }
    if ((M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200)) && (checkSetupTime == 0 || checkSetupTime == -1)) {
      checkSetupTime--;
      if (!checkDayInMonth(31, setup_month)) {
        if (checkDayInMonth(30, setup_month)) {
          if (setup_day == 31) setup_day = 30;
        } else {
          if (setup_year % 4 == 0 && setup_day > 29) setup_day = 29;
          else if (setup_year % 4 != 0 && setup_day > 28) setup_day = 28;
        }
      }
      displayRefresh();
      displaySetupDateAndTime();
    }
  }
}
