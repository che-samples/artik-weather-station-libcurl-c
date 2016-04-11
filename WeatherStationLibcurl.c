#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#define HIGH 1
#define LOW 0
#define INPUT 1
#define OUTPUT 0

// Limit the number of times the main loop is to be run. 
int currentRun = 0;
const int MAX_RUNS = 1000;

int inputPin = 7;
char* samiUrl = "https://api.samsungsami.io/v1.1/messages";
char* device_id="INSERT_YOUR_DEVICE_ID_HERE";
char* device_token="INSERT_YOUR_DEVICE_TOKEN_HERE";

bool digitalPinMode(int pin, int dir){
  FILE * fd;
  char fName[128];
  
  // Exporting the pin to be used
  if(( fd = fopen("/sys/class/gpio/export", "w")) == NULL) {
    printf("Error: unable to export pin\n");
    return false;
  }
  fprintf(fd, "%d\n", pin);
  fclose(fd);

  // Setting direction of the pin
  sprintf(fName, "/sys/class/gpio/gpio%d/direction", pin);
  if((fd = fopen(fName, "w")) == NULL) {
    printf("Error: can't open pin direction\n");
    return false;
  }
  if(dir == OUTPUT) {
    fprintf(fd, "out\n");
  } else {
    fprintf(fd, "in\n");
  }
  fclose(fd);

  return true;
}

int analogRead(int pin) {
  FILE * fd;
  char fName[64];
  char val[8];

  // open value file
  sprintf(fName, "/sys/devices/12d10000.adc/iio:device0/in_voltage%d_raw", pin);
  if((fd = fopen(fName, "r")) == NULL) {
    printf("Error: can't open analog voltage value\n");
    return 0;
  }
  fgets(val, 8, fd);
  fclose(fd);

  return atoi(val);
}

int digitalRead(int pin) {
  FILE * fd;
  char fName[128];
  char val[2];

  // Open pin value file
  sprintf(fName, "/sys/class/gpio/gpio%d/value", pin);
  if((fd = fopen(fName, "r")) == NULL) {
    printf("Error: can't open pin value\n");
    return false;
  }
  fgets(val, 2, fd);
  fclose(fd);

  return atoi(val);
}

bool digitalWrite(int pin, int val) {
  FILE * fd;
  char fName[128];

  // Open pin value file
  sprintf(fName, "/sys/class/gpio/gpio%d/value", pin);
  if((fd = fopen(fName, "w")) == NULL) {
    printf("Error: can't open pin value\n");
    return false;
  }
  if(val == HIGH) {
    fprintf(fd, "1\n");
  } else {
    fprintf(fd, "0\n");
  }
  fclose(fd);

  return true;
}

//HTTP Post to SAMI 
void runPostToSAMI(float temperature) {
  CURL *curl;
  CURLcode res;

  printf("Running Post...\n");

  curl = curl_easy_init();
  
  if (curl) {
    /* construct http header */
    struct curl_slist *requestHeader = NULL;
    char bearer[60]="";
    requestHeader = curl_slist_append(requestHeader, "Content-Type: application/json");
    sprintf(bearer, "Authorization: Bearer %s", device_token);
    requestHeader = curl_slist_append(requestHeader, bearer);

    char requestBody[256]=""; 
    sprintf(requestBody,"{\n  \"sdid\": \"%s\",\n  \"type\": \"message\",\n  \"data\": {\n        \"temperature\": %f     			           \n   }\n}", device_id, temperature);

    curl_easy_setopt(curl, CURLOPT_URL, samiUrl);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, requestHeader);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) 
      fprintf(stderr, "curl_easy_perform() failed: %s\n", 
              curl_easy_strerror(res));

    curl_slist_free_all(requestHeader);
    curl_easy_cleanup(curl);
  }

  return;
} 

int setup() {
   return 0;
}

int main(void) {
  if (setup() == 1)
  {
    exit(1);
  }
	
  while(currentRun < MAX_RUNS){
    int sensorVal = analogRead(inputPin);
    printf("current sensor is %f\n", sensorVal);
    float voltage = sensorVal * 5.0;
    voltage /= 1024.0; 

    float temperatureC = (voltage - 0.5) * 10;
    float temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;

    printf("current temperature is %f\n", temperatureF);

    currentRun++;
    runPostToSAMI(temperatureF);

    sleep(10);
  }

  return 0;
}

