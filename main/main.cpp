#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "crypto/includes.h"

#include "crypto/common.h"
#include "crypto/aes.h"
#include "crypto/aes_wrap.h"
#include "mbedtls/aes.h"

#include "BLEDevice.h"

extern "C" 
{
    void app_main();
}

#define INPUT_LENGTH        16
#define SERVICE_UUID        0x1234
#define OPEN_LOCK_UUID      0x9999

unsigned char key[] = {0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

unsigned char input[INPUT_LENGTH] = {0};
static uint32_t IV = 1;
mbedtls_aes_context aes;

/**
 * Prepare advertising data and start advertising
 * IV and advertised data changes every time peer device disconnect
 */
static void start_advertising()
{
    BLEAdvertisementData* pAdvertisingData;
    pAdvertisingData = new BLEAdvertisementData();

    char iv[16];
    sprintf(iv, "%d", IV);
    std::string data(iv, strlen(iv));
    pAdvertisingData->setServiceData(BLEUUID((uint16_t)0x2468), data);

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAdvertisementData(*pAdvertisingData);
    pAdvertising->start();
    delete(pAdvertisingData);
}

static void encrypt_msg(uint8_t* msg)
{
    unsigned char iv[16];
    sprintf((char*)iv, "test IV key %d", IV);
    unsigned char encrypt_output[INPUT_LENGTH] = {0};
    size_t iv_offset = 0;
    mbedtls_aes_crypt_cfb128(&aes, MBEDTLS_AES_ENCRYPT, INPUT_LENGTH, &iv_offset, &iv[0], msg, encrypt_output);
    ESP_LOG_BUFFER_HEX(__func__, encrypt_output, INPUT_LENGTH);
}

static void decrypt_msg(uint8_t* msg, unsigned char* decrypt_output)
{
    unsigned char iv[16];
    sprintf((char*)iv, "test IV key %d", IV);
    ESP_LOG_BUFFER_HEX("IV", iv, INPUT_LENGTH);
    size_t iv_offset = 0;
    mbedtls_aes_crypt_cfb128(&aes, MBEDTLS_AES_DECRYPT, 4, &iv_offset, &iv[0], msg, decrypt_output);
    ESP_LOG_BUFFER_HEX(__func__, decrypt_output, 4);
    ESP_LOGI(__func__, "%s", decrypt_output);
}

/**
 * Authenticate if first part of message contains actual IV key
 */
bool authenticate_device(uint8_t* data)
{
    unsigned char decrypt_output[INPUT_LENGTH] = {0};
    uint8_t auth[16];
    memcpy(&auth, data, 4);
    decode_msg(auth, &decrypt_output[0]);
    ESP_LOG_BUFFER_HEX(__func__, decrypt_output, INPUT_LENGTH*2);
    ESP_LOGI(__func__, "%s", decrypt_output);
    return true;
}

class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(){}

    void onDisconnect(BLEServer* pServer)
    {
        IV++;
        start_advertising();
    }
};

class CharCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pChar)
    {
        uint8_t* data = pChar->getData();
        // for testing purpose encrypt test message
        uint8_t test[] = {0x74, 0x65, 0x73, 0x74}; // test
        encrypt_msg(test);

        // authenticate device and perform action
        if(authenticate_device(data))
        {
            // PERFORM ACTION (OPEN DOORS???)
        }
        else
        {
            // DISCONNECT DEVICE???
        }
        
    }
};

void setup_BLE()
{
    BLEDevice::init("TEST_APP");
    // BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM); // issue to bind with samsung smartphone, reported on github
    BLEDevice::setMTU(100);

    BLEServer* pServer = BLEDevice::createServer();
    BLEService* pService = pServer->createService((uint16_t)SERVICE_UUID);
    pServer->setCallbacks(new ServerCallbacks());

    BLECharacteristic* pOpenLockerCharacteristic = pService->createCharacteristic((uint16_t) OPEN_LOCK_UUID, BLECharacteristic::PROPERTY_WRITE);
    pOpenLockerCharacteristic->setCallbacks(new CharCallbacks());

    pService->start();
    start_advertising();
}

void app_main()
{
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 256);
    setup_BLE();
}

