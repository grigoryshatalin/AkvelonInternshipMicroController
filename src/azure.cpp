// C99 libraries
#include <cstdlib>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// Libraries for MQTT client, WiFi connection and SAS-token generation.
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include <bearssl/bearssl.h>
#include <bearssl/bearssl_hmac.h>
#include <libb64/cdecode.h>

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

#include "azure.h"

// Translate secrets.h defines into variables
const char *ssid = IOT_CONFIG_WIFI_SSID;
const char *password = IOT_CONFIG_WIFI_PASSWORD;
const char *host = IOT_CONFIG_IOTHUB_FQDN;
const char *device_id = IOT_CONFIG_DEVICE_ID;
const char *device_key = IOT_CONFIG_DEVICE_KEY;
const int port = 8883;

// Memory allocated for the variables and structures.
WiFiClientSecure wifi_client;
X509List cert((const char *)ca_pem);
PubSubClient mqtt_client(wifi_client);
az_iot_hub_client client;
char sas_token[200];
uint8_t signature[512];
unsigned char encrypted_signature[32];
char base64_decoded_device_key[32];
unsigned long next_telemetry_send_time_ms = 0;
char telemetry_topic[128];
uint8_t telemetry_payload[500];
uint32_t telemetry_send_count = 0;

void azure_setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    establishConnection();
}

void azure_loop(char *payload)
{
    if (millis() > next_telemetry_send_time_ms)
    {
        // Check if connected, reconnect if needed.
        if (!mqtt_client.connected())
        {
            establishConnection();
        }

        sendTelemetry(payload);
        next_telemetry_send_time_ms = millis() + IOT_CONFIG_TELEMETRY_FREQUENCY_MS;
    }

    // MQTT loop must be called to process Device-to-Cloud and Cloud-to-Device.
    mqtt_client.loop();
}

// Auxiliary functions
void connectToWiFi()
{
    Serial.println();
    Serial.print("Connecting to WIFI SSID ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.print("WiFi connected, IP address: ");
    Serial.println(WiFi.localIP());
}

void initializeTime()
{
    Serial.print("Setting time using SNTP");

    configTime(-5 * 3600, 0, NTP_SERVERS);
    time_t now = time(NULL);
    while (now < 1510592825)
    {
        delay(500);
        Serial.print(".");
        now = time(NULL);
    }
    Serial.println("done!");
}

char *getCurrentLocalTimeString()
{
    time_t now = time(NULL);
    return ctime(&now);
}

void printCurrentTime()
{
    Serial.print("Current time: ");
    Serial.print(getCurrentLocalTimeString());
}

void receivedCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Received [");
    Serial.print(topic);
    Serial.print("]: ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println("");
}

void initializeClients()
{
    az_iot_hub_client_options options = az_iot_hub_client_options_default();
    options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

    wifi_client.setTrustAnchors(&cert);
    if (az_result_failed(az_iot_hub_client_init(
            &client,
            az_span_create((uint8_t *)host, strlen(host)),
            az_span_create((uint8_t *)device_id, strlen(device_id)),
            &options)))
    {
        Serial.println("Failed initializing Azure IoT Hub client");
        return;
    }

    mqtt_client.setServer(host, port);
    mqtt_client.setCallback(receivedCallback);
}

uint32_t getSecondsSinceEpoch() { return (uint32_t)time(NULL); }

int generateSasToken(char *sas_token, size_t size)
{
    az_span signature_span = az_span_create((uint8_t *)signature, sizeofarray(signature));
    az_span out_signature_span;
    az_span encrypted_signature_span = az_span_create((uint8_t *)encrypted_signature, sizeofarray(encrypted_signature));

    uint32_t expiration = getSecondsSinceEpoch() + ONE_HOUR_IN_SECS;

    // Get signature
    if (az_result_failed(az_iot_hub_client_sas_get_signature(
            &client, expiration, signature_span, &out_signature_span)))
    {
        Serial.println("Failed getting SAS signature");
        return 1;
    }

    // Base64-decode device key
    int base64_decoded_device_key_length = base64_decode_chars(device_key, strlen(device_key), base64_decoded_device_key);

    if (base64_decoded_device_key_length == 0)
    {
        Serial.println("Failed base64 decoding device key");
        return 1;
    }

    // SHA-256 encrypt
    br_hmac_key_context kc;
    br_hmac_key_init(
        &kc, &br_sha256_vtable, base64_decoded_device_key, base64_decoded_device_key_length);

    br_hmac_context hmac_ctx;
    br_hmac_init(&hmac_ctx, &kc, 32);
    br_hmac_update(&hmac_ctx, az_span_ptr(out_signature_span), az_span_size(out_signature_span));
    br_hmac_out(&hmac_ctx, encrypted_signature);

    // Base64 encode encrypted signature
    String b64enc_hmacsha256_signature = base64::encode(encrypted_signature, br_hmac_size(&hmac_ctx));

    az_span b64enc_hmacsha256_signature_span = az_span_create(
        (uint8_t *)b64enc_hmacsha256_signature.c_str(), b64enc_hmacsha256_signature.length());

    // URl-encode base64 encoded encrypted signature
    if (az_result_failed(az_iot_hub_client_sas_get_password(
            &client,
            expiration,
            b64enc_hmacsha256_signature_span,
            AZ_SPAN_EMPTY,
            sas_token,
            size,
            NULL)))
    {
        Serial.println("Failed getting SAS token");
        return 1;
    }

    return 0;
}

int connectToAzureIoTHub()
{
    size_t client_id_length;
    char mqtt_client_id[128];
    if (az_result_failed(az_iot_hub_client_get_client_id(
            &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
    {
        Serial.println("Failed getting client id");
        return 1;
    }

    mqtt_client_id[client_id_length] = '\0';

    char mqtt_username[128];
    // Get the MQTT user name used to connect to IoT Hub
    if (az_result_failed(az_iot_hub_client_get_user_name(
            &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
    {
        printf("Failed to get MQTT clientId, return code\n");
        return 1;
    }

    Serial.print("Client ID: ");
    Serial.println(mqtt_client_id);

    Serial.print("Username: ");
    Serial.println(mqtt_username);

    mqtt_client.setBufferSize(MQTT_PACKET_SIZE);

    while (!mqtt_client.connected())
    {
        time_t now = time(NULL);

        Serial.print("MQTT connecting ... ");

        if (mqtt_client.connect(mqtt_client_id, mqtt_username, sas_token))
        {
            Serial.println("connected.");
        }
        else
        {
            Serial.print("failed, status code =");
            Serial.print(mqtt_client.state());
            Serial.println(". Trying again in 5 seconds.");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }

    mqtt_client.subscribe(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC);

    return 0;
}

void establishConnection()
{
    connectToWiFi();
    initializeTime();
    printCurrentTime();
    initializeClients();

    // The SAS token is valid for 1 hour by default.
    // After one hour the code must be restarted, or the client won't be able
    // to connect/stay connected to the Azure IoT Hub.
    if (generateSasToken(sas_token, sizeofarray(sas_token)) != 0)
    {
        Serial.println("Failed generating MQTT password");
    }
    else
    {
        connectToAzureIoTHub();
    }

    digitalWrite(LED_PIN, LOW);
}

char *getTelemetryPayload(char *payload)
{
    az_span temp_span = az_span_create(telemetry_payload, sizeof(telemetry_payload));
    temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("{ \"msgCount\": "));
    (void)az_span_u32toa(temp_span, telemetry_send_count++, &temp_span);
    temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR(", "));
    az_span extraJSON = az_span_create_from_str(payload);
    temp_span = az_span_copy(temp_span, extraJSON);
    temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR(" }"));
    temp_span = az_span_copy_u8(temp_span, '\0');

    return (char *)telemetry_payload;
}

void sendTelemetry(char *payload)
{
    digitalWrite(LED_PIN, HIGH);
    Serial.print(millis());
    Serial.print(" ESP8266 Sending telemetry . . . ");
    if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
            &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
    {
        Serial.println("Failed az_iot_hub_client_telemetry_get_publish_topic");
        return;
    }

    mqtt_client.publish(telemetry_topic, getTelemetryPayload(payload), false);
    Serial.println("OK");
    delay(100);
    digitalWrite(LED_PIN, LOW);
}