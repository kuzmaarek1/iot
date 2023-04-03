#include "M5Core2.h"
#include <Arduino.h>
#include <WiFi.h>
#include "time.h"

const char* ssid = "ANIA";
const char* password = "5249555";
const char* ntpServer = "tempus1.gum.gov.pl";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
int checkWiFiCount = 0;
int checkWiFi = 0;
int checkSetupTime = 0;
int isSetupFirst = 1;
int setup_hours = 0, setup_minutes = 0, setup_day = 1, setup_month = 1, setup_year = 2023;
int start_setup = 1;

RTC_TimeTypeDef RTCtime;
RTC_DateTypeDef RTCDate;

char timeStrbuff[64];
char dateStrbuff[64];

int searchEvent[7];

int current_year, current_month, current_data;

int current_page = 0;
int current_main_page = 0;
int current_add_page = 0;
int current_edit_page = 0;

int current_events_count = 1;

int start_display_main = 1;
int start_display_add = 1;
int start_display_edit = 1;
int start_display_details = 1;

int changeDate = 1;
int stringCount = 0;
int indexString;
int lineRead;

int *dayRead, *monthRead, *yearRead, *startHoursRead, *startMinutesRead, *endHoursRead, *endMinutesRead, *typesRead;
String strRead;
File file;

Button detailsBtn(110, 160, 150, 20, "detailsBtn");
Button addBtn(110, 200, 100, 30, "addBtn");
Button deleteBtn(10, 200, 100, 30, "deleteBtn");
Button editBtn(210, 200, 100, 30, "editBtn");

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

int *all_event_current_startHoursRead, *all_event_current_startMinutesRead, *all_event_current_endHoursRead, *all_event_current_endMinutesRead, *all_event_current_typesRead;
int all_event_current_day;

int deleteError;
int sortedTypesRead, sortedDayRead, sortedMonthRead, sortedYearRead, sortedStartHoursRead, sortedStartMinutesRead, sortedEndHoursRead, sortedEndMinutesRead;

Button event_add1(210, 90, 40, 35);
Button event_sub1(260, 90, 40, 35);
Button event_add2(210, 140, 40, 35);
Button event_sub2(260, 140, 40, 35);
Button event_add3(210, 190, 40, 35);
Button event_sub3(260, 190, 40, 35);

Button setupBtn(0, 0, 320, 10);

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
      checkSetupTime = 1;
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

    M5.Lcd.setCursor(110, 125);
    M5.Lcd.print(event_type[all_event_current_typesRead[current_events_count - 1]]);

    M5.Lcd.setTextSize(2);
    M5.Lcd.fillRect(110, 160, 150, 20, RED);
    M5.Lcd.setTextColor(WHITE, RED);
    M5.Lcd.setCursor(132, 162);
    M5.Lcd.print("Szczegoly");
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

      M5.Lcd.fillRect(110, 200, 100, 30, BLUE);
      M5.Lcd.setCursor(132, 208);
      M5.Lcd.setTextColor(WHITE, BLUE);
      M5.Lcd.print("Dodaj");
      M5.Lcd.setTextColor(WHITE, BLACK);
      break;
  }
}

void displayEditPage() {
  M5.Lcd.setCursor(50, 70);
  M5.Lcd.print("EDYTOWANIE");

  switch (current_edit_page) {
    case 0:
      M5.Lcd.setCursor(250, 50);
      M5.Lcd.print("1/4");
      M5.Lcd.setCursor(50, 100);
      M5.Lcd.print("Data");
      break;
    case 1:
      M5.Lcd.setCursor(250, 50);
      M5.Lcd.print("2/4");
      M5.Lcd.setCursor(50, 100);
      M5.Lcd.print("Czas rozpoczecia");
      break;
    case 2:
      M5.Lcd.setCursor(250, 50);
      M5.Lcd.print("3/4");
      M5.Lcd.setCursor(50, 100);
      M5.Lcd.print("Czas rozpoczecia");
      break;
    case 3:
      M5.Lcd.setCursor(250, 50);
      M5.Lcd.print("4/4");
      M5.Lcd.setCursor(50, 100);
      M5.Lcd.print("Rodzaj wydarzenia");
      break;
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
  M5.Lcd.fillRect(10, 200, 100, 30, BLUE);
  M5.Lcd.setCursor(40, 210);
  M5.Lcd.print("Usun");
  M5.Lcd.fillRect(210, 200, 100, 30, BLUE);
  M5.Lcd.setCursor(232, 210);
  M5.Lcd.print("Edytuj");

  M5.Lcd.setTextColor(WHITE, BLACK);
}

void setupParametres() {
  current_main_page = 0;
  current_add_page = 0;
  current_edit_page = 0;
  current_events_count = 1;

  start_display_main = 1;
  start_display_add = 1;
  start_display_edit = 1;
  start_display_details = 1;
}

void handleDetails(Event& e) {
  if (current_page == 0) {
    current_page = 2;
    start_display_main = 1;
    start_display_details = 1;
    displayRefresh();
  }
}

void handleAdd(Event& e) {
  if (checkSetupTime == -1) {
    setupTime(setup_hours, setup_minutes, 0);
    setupData(setup_year, setup_month, setup_day);
    setupParametres();
    changeDate = 0;
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
      if (file.printf("%d;%d;%d;%d;%d;%d;%d;%d\n",
                      event_day, event_month, event_year, event_hours_start, event_minutes_start, event_hours_end, event_minutes_end, event_type_number)) {
        displayRefresh();
        M5.Lcd.setTextColor(GREEN, BLACK);
        M5.Lcd.setCursor(120, 110);
        M5.Lcd.print("DODANO");
        delay(2000);
        M5.Rtc.GetDate(&RTCDate);
        setupCurrentData(RTCDate.Year, RTCDate.Month, RTCDate.Date);
        delay(2000);
        displayRefresh();
      } else {
        displayRefresh();
        M5.Lcd.setCursor(5, 110);
        M5.Lcd.setTextColor(RED, BLACK);
        M5.Lcd.print("BLAD. SPROBOJ JESZCZE RAZ!");
        delay(2000);
        displayRefresh();
      }
      file.close();
      setupParametres();
      current_page = 0;
      displayRefresh();
    }
  }
}

void handleEdit(Event& e) {
  if (current_page == 2) {
    current_page = 3;
    start_display_details = 1;
    start_display_edit = 1;
    displayRefresh();
  }
}

void handleDelete(Event& e) {
  if (current_page == 2) {

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
      if (startHoursRead[i] > startHoursRead[j] || (startHoursRead[i] == startHoursRead[j] && startMinutesRead[i] > startMinutesRead[j])) {
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
  if (checkSetupTime == 0) {
    if (setup_day < 31) {
      setup_day++;
      displayRefresh();
      start_setup = 1;
    }
  } else if (checkSetupTime == -1) {
    if (setup_hours < 23) {
      setup_hours++;
      displayRefresh();
      start_setup = 1;
    }
  } else if (current_page == 1) {
    if (current_add_page == 0) {
      if (event_day < 31) {
        event_day++;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 1) {
      if (event_hours_start < 23) {
        event_hours_start++;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 2) {
      if (event_hours_end < 23) {
        event_hours_end++;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 3) {
      if (event_type_number != 0) {
        event_type_number--;
        displayRefresh();
        start_display_add = 1;
      }
    }
  }
}

void handleEventSub1(Event& e) {
  if (checkSetupTime == 0) {
    if (setup_day != 1) {
      setup_day = 1;
      displayRefresh();
      start_setup = 1;
    }
  } else if (checkSetupTime == -1) {
    if (setup_hours != 0) {
      setup_hours--;
      displayRefresh();
      start_setup = 1;
    }
  } else if (current_page == 1) {
    if (current_add_page == 0) {
      if (event_day != 1) {
        event_day--;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 1) {
      if (event_hours_start != 0) {
        event_hours_start--;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 2) {
      if (event_hours_end != 0) {
        event_hours_end--;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 3) {
      if (event_type_number < 14) {
        event_type_number++;
        displayRefresh();
        start_display_add = 1;
      }
    }
  }
}

void handleEventAdd2(Event& e) {
  if (checkSetupTime == 0) {
    if (setup_month < 12) {
      setup_month++;
      displayRefresh();
      start_setup = 1;
    }
  } else if (checkSetupTime == -1) {
    if (setup_minutes < 59) {
      setup_minutes++;
      displayRefresh();
      start_setup = 1;
    }
  } else if (current_page == 1) {
    if (current_add_page == 0) {
      if (event_month < 12) {
        event_month++;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 1) {
      if (event_minutes_start < 59) {
        event_minutes_start++;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 2) {
      if (event_minutes_end < 59) {
        event_minutes_end++;
        displayRefresh();
        start_display_add = 1;
      }
    }
  }
}

void handleEventSub2(Event& e) {
  if (checkSetupTime == 0) {
    if (setup_month != 1) {
      setup_month--;
      displayRefresh();
      start_setup = 1;
    }
  } else if (checkSetupTime == -1) {
    if (setup_minutes != 0) {
      setup_minutes--;
      displayRefresh();
      start_setup = 1;
    }
  } else if (current_page == 1) {
    if (current_add_page == 0) {
      if (event_month != 1) {
        event_month--;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 1) {
      if (event_minutes_start != 0) {
        event_minutes_start--;
        displayRefresh();
        start_display_add = 1;
      }
    } else if (current_add_page == 2) {
      if (event_minutes_end != 0) {
        event_minutes_end--;
        displayRefresh();
        start_display_add = 1;
      }
    }
  }
}

void handleEventAdd3(Event& e) {
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

void handleEventSub3(Event& e) {
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
      M5.Lcd.print("1/2");
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
      M5.Lcd.print("2/2");
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
      M5.Lcd.setCursor(50, 178);
      M5.Lcd.print("Data");
      M5.Lcd.setCursor(150, 178);
      M5.Lcd.printf("%02d.%02d.%04d", setup_day, setup_month, setup_year);

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

void setup() {

  M5.begin();
  setupWifi();
  setTimeAndData();

  delay(1000);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);

  detailsBtn.addHandler(handleDetails, E_TOUCH);
  addBtn.addHandler(handleAdd, E_TOUCH);
  editBtn.addHandler(handleEdit, E_TOUCH);
  deleteBtn.addHandler(handleDelete, E_TOUCH);

  event_add1.addHandler(handleEventAdd1, E_TOUCH);
  event_sub1.addHandler(handleEventSub1, E_TOUCH);

  event_add2.addHandler(handleEventAdd2, E_TOUCH);
  event_sub2.addHandler(handleEventSub2, E_TOUCH);

  event_add3.addHandler(handleEventAdd3, E_TOUCH);
  event_sub3.addHandler(handleEventSub3, E_TOUCH);

  setupBtn.addHandler(handleSetup, E_TOUCH);

  Serial.begin(115200);
  if (!SD.begin(5)) {
    Serial.println("Card Mount Failed");
  }
}

void loop() {
  M5.update();
  if (checkSetupTime > 0) {
    displayTime();
    displayDate();

    M5.Rtc.GetTime(&RTCtime);
    M5.Rtc.GetDate(&RTCDate);
    isSetupFirst = 0;

    if (RTCtime.Hours == 0 && RTCtime.Minutes == 0 && changeDate) {
      setupParametres();
      setupData(RTCDate.Year, RTCDate.Month, RTCDate.Date);
      delay(1000);
      changeDate = 0;
      current_page = 0;
      displayRefresh();
    }

    if (RTCtime.Hours == 0 && RTCtime.Minutes == 1 && changeDate) {
      changeDate = 1;
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
                else if (event_day > 28) event_day = 28;
              }
            }
            if (current_add_page == 1 && event_year == RTCDate.Year && (event_month < RTCDate.Month || (event_month == RTCDate.Month && event_day < RTCDate.Date))) {
              event_day = RTCDate.Date;
              event_month = RTCDate.Month;
              event_year = RTCDate.Year;
            }
            if (current_add_page == 3 && ((event_hours_end == event_hours_start && event_minutes_end == event_minutes_start) || event_hours_end < event_hours_start || (event_hours_end == event_hours_start && event_minutes_end < event_minutes_start))) {
              event_hours_end = 23;
              event_minutes_end = 59;
            } else if (RTCDate.Year == event_year && RTCDate.Month == event_month && RTCDate.Date == event_day && current_add_page == 3) {
              if (event_hours_end < RTCtime.Hours || (event_hours_end == RTCtime.Hours && event_minutes_end < RTCtime.Minutes)) {
                event_hours_end = RTCtime.Hours;
                event_minutes_end = RTCtime.Minutes;
              }
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
      case 3:  //edit_page
        if (start_display_edit) {
          displayEditPage();
          start_display_edit = 0;
        }
        if ((M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))) {
          if (current_edit_page == 0) {
            current_page = 2;
            start_display_edit = 1;
            displayRefresh();
          } else {
            current_edit_page--;
            displayRefresh();
            displayEditPage();
          }
        }
        if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200)) {
          if (current_edit_page < 3) {
            current_edit_page++;
            displayRefresh();
            displayEditPage();
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
        changeDate = 0;
        current_page = 0;
        displayRefresh();
      }
    }
    if ((M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200)) && checkSetupTime == 0) {
      checkSetupTime--;
      if (!checkDayInMonth(31, setup_month)) {
        if (checkDayInMonth(30, setup_month)) {
          if (setup_day == 31) setup_day = 30;
        } else {
          if (setup_year % 4 == 0 && setup_day > 29) setup_day = 29;
          else if (setup_day > 28) setup_day = 28;
        }
      }
      displayRefresh();
      displaySetupDateAndTime();
    }
  }
}
