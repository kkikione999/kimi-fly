/**
 * @file main.cpp
 * @brief ESP32-C3 WiFi STA + UART Bridge for Drone Control
 * @note Hardware: ESP32-C3 connected to STM32F411 via UART
 *       WiFi: STA mode, connects to SSID "whc"
 *       UART: TX=GPIO5, RX=GPIO4, 115200 baud
 *       TCP Server: Port 8080 for WiFi communication
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "wifi_config.h"

/* ============================================================================
 * Global Variables
 * ============================================================================ */

static WiFiServer tcpServer(TCP_SERVER_PORT);
static WiFiClient tcpClient;
static bool clientConnected = false;

static volatile uint32_t txCounter = 0;
static volatile uint32_t rxCounter = 0;
static volatile uint32_t wifiTxCounter = 0;
static volatile uint32_t wifiRxCounter = 0;

static uint8_t uartRxBuffer[UART_ST_BUF_SIZE];
static uint8_t tcpRxBuffer[TCP_BUF_SIZE];

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Calculate checksum for a message
 */
static uint8_t calculateChecksum(const ProtocolMessage *msg)
{
    uint8_t checksum = 0;
    const uint8_t *data = (const uint8_t *)msg;
    uint8_t length = sizeof(ProtocolHeader) + msg->header.length;

    for (uint8_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Build a protocol message
 */
static int buildMessage(ProtocolMessage *msg, uint8_t type,
                        const uint8_t *payload, uint8_t payloadLen)
{
    if (payloadLen > PROTOCOL_MAX_PAYLOAD) {
        return -1;
    }

    msg->header.header = PROTOCOL_HEADER;
    msg->header.type = type;
    msg->header.length = payloadLen;

    if (payloadLen > 0 && payload != NULL) {
        memcpy(msg->payload, payload, payloadLen);
    }

    msg->checksum = calculateChecksum(msg);

    return sizeof(ProtocolHeader) + payloadLen + 1; // +1 for checksum
}

/**
 * @brief Send message to STM32 via UART
 */
static void sendToSTM32(const ProtocolMessage *msg, size_t len)
{
    UART_ST_PORT.write((const uint8_t *)msg, len);
    UART_ST_PORT.flush();
    txCounter++;
}

/**
 * @brief Send message to WiFi client
 */
static void sendToWiFi(const ProtocolMessage *msg, size_t len)
{
    if (clientConnected && tcpClient.connected()) {
        tcpClient.write((const uint8_t *)msg, len);
        wifiTxCounter++;
    }
}

/**
 * @brief Broadcast status message to both UART and WiFi
 */
static void broadcastStatus(const char *statusText)
{
    ProtocolMessage msg;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];

    strncpy((char *)payload, statusText, PROTOCOL_MAX_PAYLOAD - 1);
    payload[PROTOCOL_MAX_PAYLOAD - 1] = '\0';

    int len = buildMessage(&msg, MSG_TYPE_STATUS, payload, strlen((char *)payload));
    if (len > 0) {
        sendToSTM32(&msg, len);
        sendToWiFi(&msg, len);
    }
}

/**
 * @brief Send WiFi status message
 */
static void sendWiFiStatus()
{
    ProtocolMessage msg;
    uint8_t payload[32];

    String status = "WiFi:";
    if (WiFi.status() == WL_CONNECTED) {
        status += "CONNECTED IP:";
        status += WiFi.localIP().toString();
    } else {
        status += "DISCONNECTED";
    }

    strncpy((char *)payload, status.c_str(), sizeof(payload) - 1);
    payload[sizeof(payload) - 1] = '\0';

    int len = buildMessage(&msg, MSG_TYPE_WIFI_STATUS, payload, strlen((char *)payload));
    if (len > 0) {
        sendToSTM32(&msg, len);
    }
}

/* ============================================================================
 * WiFi Functions
 * ============================================================================ */

/**
 * @brief Initialize WiFi in STA mode
 */
static bool initWiFi()
{
    Serial.println("[WiFi] Initializing STA mode...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);

    Serial.print("[WiFi] Connecting to ");
    Serial.print(WIFI_STA_SSID);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < WIFI_MAX_RETRIES) {
        delay(1000);
        Serial.print(".");
        retries++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Connected successfully!");
        Serial.print("[WiFi] IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("[WiFi] MAC Address: ");
        Serial.println(WiFi.macAddress());
        Serial.print("[WiFi] RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        return true;
    } else {
        Serial.println("[WiFi] Connection failed!");
        return false;
    }
}

/**
 * @brief Reconnect WiFi if disconnected
 */
static void checkWiFiConnection()
{
    static unsigned long lastCheck = 0;
    unsigned long now = millis();

    if (now - lastCheck < 5000) {
        return;
    }
    lastCheck = now;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Connection lost, reconnecting...");
        WiFi.reconnect();
    }
}

/* ============================================================================
 * TCP Server Functions
 * ============================================================================ */

/**
 * @brief Initialize TCP server
 */
static void initTCPServer()
{
    tcpServer.begin();
    Serial.print("[TCP] Server started on port ");
    Serial.println(TCP_SERVER_PORT);
}

/**
 * @brief Handle TCP client connections and data
 */
static void handleTCPClient()
{
    // Check for new client connection
    if (!clientConnected || !tcpClient.connected()) {
        if (tcpServer.hasClient()) {
            if (tcpClient) {
                tcpClient.stop();
            }
            tcpClient = tcpServer.available();
            clientConnected = true;
            Serial.println("[TCP] Client connected");
            broadcastStatus("WiFi client connected");
        }
    }

    // Handle disconnection
    if (clientConnected && !tcpClient.connected()) {
        clientConnected = false;
        Serial.println("[TCP] Client disconnected");
        broadcastStatus("WiFi client disconnected");
        tcpClient.stop();
        return;
    }

    // Read data from TCP client and forward to UART
    if (clientConnected && tcpClient.available()) {
        int len = tcpClient.read(tcpRxBuffer, sizeof(tcpRxBuffer));
        if (len > 0) {
            wifiRxCounter += len;
            // Forward to UART (STM32)
            UART_ST_PORT.write(tcpRxBuffer, len);
            UART_ST_PORT.flush();

            Serial.print("[Bridge] WiFi->UART: ");
            Serial.print(len);
            Serial.println(" bytes");
        }
    }
}

/* ============================================================================
 * UART Functions
 * ============================================================================ */

/**
 * @brief Initialize UART for STM32 communication
 */
static void initUART()
{
    UART_ST_PORT.begin(UART_ST_BAUDRATE, SERIAL_8N1, UART_ST_RX_PIN, UART_ST_TX_PIN);

    Serial.println("[UART] Initialized:");
    Serial.print("  TX Pin: GPIO");
    Serial.println(UART_ST_TX_PIN);
    Serial.print("  RX Pin: GPIO");
    Serial.println(UART_ST_RX_PIN);
    Serial.print("  Baudrate: ");
    Serial.println(UART_ST_BAUDRATE);
}

/**
 * @brief Handle UART data from STM32
 */
static void handleUART()
{
    while (UART_ST_PORT.available()) {
        int len = UART_ST_PORT.read(uartRxBuffer, sizeof(uartRxBuffer));
        if (len > 0) {
            rxCounter += len;

            // Forward to WiFi client if connected
            if (clientConnected && tcpClient.connected()) {
                tcpClient.write(uartRxBuffer, len);
                wifiTxCounter += len;
            }

            // Debug output
            Serial.print("[Bridge] UART->WiFi: ");
            Serial.print(len);
            Serial.println(" bytes");

            // Print first few bytes for debugging
            if (len >= 4) {
                Serial.print("[Bridge] Data: ");
                for (int i = 0; i < min(len, 8); i++) {
                    Serial.printf("%02X ", uartRxBuffer[i]);
                }
                Serial.println();
            }
        }
    }
}

/* ============================================================================
 * Setup and Loop
 * ============================================================================ */

void setup()
{
    // Initialize USB serial for debugging
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n================================");
    Serial.println("Kimi-Fly ESP32-C3 WiFi Bridge");
    Serial.println("Target: STM32F411CEU6");
    Serial.println("================================");

    // Initialize UART for STM32 communication
    initUART();

    // Initialize WiFi
    bool wifiOk = initWiFi();

    // Initialize TCP server
    initTCPServer();

    // Send initial status
    delay(100);
    if (wifiOk) {
        broadcastStatus("ESP32 initialized, WiFi connected");
        sendWiFiStatus();
    } else {
        broadcastStatus("ESP32 initialized, WiFi failed");
    }

    Serial.println("[Main] Initialization complete");
}

void loop()
{
    static unsigned long lastHeartbeat = 0;
    static unsigned long lastStats = 0;
    static unsigned long lastWiFiStatus = 0;

    unsigned long now = millis();

    // Handle UART data
    handleUART();

    // Handle TCP client
    handleTCPClient();

    // Check WiFi connection
    checkWiFiConnection();

    // Send heartbeat every 2 seconds
    if (now - lastHeartbeat >= 2000) {
        lastHeartbeat = now;

        ProtocolMessage msg;
        uint8_t payload[4] = {
            (uint8_t)(now >> 24),
            (uint8_t)(now >> 16),
            (uint8_t)(now >> 8),
            (uint8_t)(now)
        };

        int len = buildMessage(&msg, MSG_TYPE_HEARTBEAT, payload, sizeof(payload));
        if (len > 0) {
            sendToSTM32(&msg, len);
            sendToWiFi(&msg, len);
        }
    }

    // Send statistics every 10 seconds
    if (now - lastStats >= 10000) {
        lastStats = now;

        Serial.println("\n=== Statistics ===");
        Serial.print("UART TX: ");
        Serial.println(txCounter);
        Serial.print("UART RX: ");
        Serial.println(rxCounter);
        Serial.print("WiFi TX: ");
        Serial.println(wifiTxCounter);
        Serial.print("WiFi RX: ");
        Serial.println(wifiRxCounter);
        Serial.print("WiFi Status: ");
        Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        Serial.print("Client: ");
        Serial.println(clientConnected ? "Connected" : "None");
        Serial.println("==================\n");
    }

    // Send WiFi status every 30 seconds
    if (now - lastWiFiStatus >= 30000) {
        lastWiFiStatus = now;
        sendWiFiStatus();
    }

    // Small delay to prevent watchdog issues
    delay(1);
}
