#include <EdisonAWSImplementations.h>
#include <AmazonDynamoDBClient.h>
#include "keys.h"
#include <Utils.h>
#include <WiFi.h>

char ssid[] = "hackers"; //  your network SSID (name) 
char pass[] = "hackers420";    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;
bool isTouched = false;

EdisonHttpClient httpClient;
EdisonDateTimeProvider dateTimeProvider;
AmazonDynamoDBClient ddbClient;
GetItemInput getItemInput;
PutItemInput putItemInput;
AttributeValue hashKey;
AttributeValue rangeKey;
ActionError actionError;

const int pinLight = A0;
const int pinButton = 3;
const int pinLed    = 7;
const int pinSpeaker = 2;

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(pinSpeaker, HIGH);
    delayMicroseconds(tone);
    digitalWrite(pinSpeaker, LOW);
    delayMicroseconds(tone);
  }
}

void setup()
{

  Serial.begin(115200);
  
  //wifi
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  }
  
  String fv = WiFi.firmwareVersion();
  if( fv != "1.1.0" )
    Serial.println("Please upgrade the firmware");
  
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
     //Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    status = WiFi.begin(ssid, pass);
  
     //wait 10 seconds for connection:
    delay(7000);
  } 
  Serial.println("Connected to wifi");
  printWifiStatus();
  
  /* Initialize ddbClient. */
  ddbClient.setAWSRegion(awsRegion);
  ddbClient.setAWSEndpoint(awsEndpoint);
  ddbClient.setAWSSecretKey(awsSecKey);
  ddbClient.setAWSKeyID(awsKeyID);
  ddbClient.setHttpClient(&httpClient);
  ddbClient.setDateTimeProvider(&dateTimeProvider);
  
  // Configure the button's pin for input signals.
  pinMode(pinButton, INPUT);

  // Configure the LED's pin for output.
  pinMode(pinLed, OUTPUT);
  pinMode(pinSpeaker, OUTPUT);
}

void loop()
{
//  Serial.println("loop");
//  int sensorValue = analogRead(pinLight);
//  Serial.println(sensorValue);
//  Serial.println("\n");

  int sensorValue = analogRead(pinLight);
  Serial.println(sensorValue+"\n");
  if (sensorValue < 400 && isTouched==false) {
    isTouched = true;
    digitalWrite(pinLed, HIGH);
    playTone(1000, 100);// tone, duration
    putDynamoDb();
  }
  else
  {
    isTouched = false;
    digitalWrite(pinLed, LOW);
  }

  if (digitalRead(pinButton))
  {
    // When the button is pressed, turn the LED on.
    digitalWrite(pinLed, HIGH);
    playTone(1275, 100);// tone, duration
    putDynamoDb();
  }
  else
  {
    // Otherwise, turn the LED off.
    digitalWrite(pinLed, LOW);
  }

  delay(100);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


void putDynamoDb() {    
  // Put device & datestamp record in DynamoDB table
  /* Create an Item. */
  AttributeValue deviceIdValue;
  deviceIdValue.setS(dataTableHashKeyValue);
  
  AttributeValue timeValue;
  /* Getting current time for Time attribute. */
  timeValue.setN(dateTimeProvider.getDateTime());
  
  AttributeValue deviceValue;
  deviceValue.setS(deviceAttributeValue);
  
  MinimalKeyValuePair < MinimalString, AttributeValue
          > att1(dataTableHashKeyName, deviceIdValue);
  MinimalKeyValuePair < MinimalString, AttributeValue
          > att2(dataTableRangeKeyName, timeValue);
  MinimalKeyValuePair < MinimalString, AttributeValue
          > att3(deviceAttributeName, deviceValue );
  MinimalKeyValuePair<MinimalString, AttributeValue> itemArray[] = { att1,
          att2, att3};

  /* Set values for putItemInput. */
  putItemInput.setItem(MinimalMap < AttributeValue > (itemArray, 3));
  putItemInput.setTableName(dataTableName);

  /* perform putItem and check for errors. */
  PutItemOutput putItemOutput = ddbClient.putItem(putItemInput,actionError);
  switch (actionError) {
  case NONE_ACTIONERROR:
      Serial.println("DynamoDB PutItem succeeded!");
      break;
  case INVALID_REQUEST_ACTIONERROR:
      Serial.print("ERROR: ");
      Serial.println(putItemOutput.getErrorMessage().getCStr());
      break;
  case MISSING_REQUIRED_ARGS_ACTIONERROR:
      Serial.println(
              "ERROR: Required arguments were not set for PutItemInput");
      break;
  case RESPONSE_PARSING_ACTIONERROR:
      Serial.println("ERROR: Problem parsing http response of PutItem");
      break;
  case CONNECTION_ACTIONERROR:
      Serial.println("ERROR: Connection problem");
      break;
  }
}
