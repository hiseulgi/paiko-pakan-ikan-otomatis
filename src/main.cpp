#include "main.h"

void setup() {
    Serial.begin(115200);

    // PINMODE
    myservo.attach(servoPin, 500, 2500);
    myservo.write(90);
    pinMode(btnPin, INPUT);
    lcd.begin();

    // KONEKSI WIFI
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    // SETTING FIREBASE
    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback;   //see addons/TokenHelper.h

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);

    Firebase.setDoubleDigits(5);
    // =============================

    // NTP BEGIN
    timeClient.begin();

    // Telegram Bot
    myBot.wifiConnect(WIFI_SSID, WIFI_PASSWORD);
    myBot.setTelegramToken(TOKEN);

    if (myBot.testConnection()) {
        Serial.println("Connected to Telegram");
    } else {
        Serial.println("Failed connect to Telegram");
    }
}

void loop() {
    // Reading button state
    btnState = digitalRead(btnPin);

    // Update nilai datetime dari internet
    timeClient.update();

    String hari = weekDays[timeClient.getDay()];
    String tanggal = timeClient.getFormattedDate();
    String waktu = timeClient.getFormattedTime();

    // Telegram Bot
    handleNewMessages(hari, tanggal, waktu);

    // if (millis() > lastTimeBotRan + bot_delay) {
    //     handleNewMessages(hari, tanggal, waktu);
    //     lastTimeBotRan = millis();
    // }

    // Handle Input dari button
    if (!btnState) {
        feedFish();
        pushHistory(hari, tanggal, waktu);
        delay(500);
    }

    delay(10);
}

// Fungsi untuk menghandle pesan yg masuk pada telegram
void handleNewMessages(String hari, String tanggal, String waktu) {
    if (myBot.getNewMessage(msg)) {
        Serial.println(msg.text);
        user = msg.sender;

        // Validasi user
        int chat_id = user.id;
        if (chat_id != ID) {
            String txt = "Maaf, Paiko tidak mengenali kak " + user.firstName + ". 🥺";
            myBot.sendMessage(chat_id, txt);
            return;
        }

        // tampilan awal telegram
        if (msg.text.equalsIgnoreCase("/start")) {
            String txt = "Selamat Datang kak " + user.firstName + " di bot Pakan Ikan Otomatis (PAIKO)! 🐟\n";
            txt += "Ketik /help untuk melihat daftar perintah ya.";
            myBot.sendMessage(chat_id, txt);
            return;
        }

        // menampilkan daftar command
        if (msg.text.equalsIgnoreCase("/help")) {
            String txt = "Berikut hal yang bisa Paiko lakukan : \n\n";
            txt += "/feed - memberi pakan ikan langsung\n";
            txt += "/settime - set waktu untuk memberi pakan ikan secara otomatis\n";
            txt += "/help - menampilkan daftar perintah\n";
            txt += "/status - menampilkan status alat";
            myBot.sendMessage(chat_id, txt);
            return;
        }

        // mengambil waktu yang disimpan di firebase
        if (msg.text.equalsIgnoreCase("/status")) {
            FirebaseJson json;
            if (Firebase.RTDB.getJSON(&fbdo, "lastPush")) {
                json = fbdo.to<FirebaseJson>().raw();
            } else {
                myBot.sendMessage(chat_id, "Maaf kak, ada error. 😥\nSilahkan coba lagi yaa!");
            }

            FirebaseJsonData result;
            json.get(result, "hari");
            String day = result.to<String>();
            Serial.println(day);

            json.get(result, "tanggal");
            String date = result.to<String>();
            Serial.println(date);

            json.get(result, "waktu");
            String time = result.to<String>();
            Serial.println(time);

            String txt = "Paiko terakhir kali memberikan pakan ikan pada : \n\n";
            txt += "Hari          : " + day;
            txt += "\nTanggal   : " + date;
            txt += "\nWaktu      : " + time;
            myBot.sendMessage(chat_id, txt);
            return;
        }

        // memberi pakan ikan langsung
        if (msg.text.equalsIgnoreCase("/feed")) {
            myBot.sendMessage(chat_id, "Tunggu sebentar ya kak! 😊");
            feedFish();
            pushHistory(hari, tanggal, waktu);
            myBot.sendMessage(chat_id, "Pakan ikan berhasil dikeluarkan!");
            return;
        }

        // set waktu untuk memberi pakan ikan secara otomatis
        if (msg.text.equalsIgnoreCase("/settime")) {
            myBot.sendMessage(chat_id, "Sabar ya kak! Fitur ini sedang dibangun hehe.");
            return;
        }

        myBot.sendMessage(chat_id, "Maaf, Paiko tidak tahu perintah ini. 🥺");
    }
}

// Fungsi untuk push data history ke firebase
void pushHistory(String hari, String tanggal, String waktu) {
    // push history
    FirebaseJson json;
    json.set("hari", hari);
    json.set("tanggal", tanggal);
    json.set("waktu", waktu);
    Firebase.pushJSON(fbdo, "history", json);

    // set lastPushed
    Firebase.setJSON(fbdo, "lastPush", json);
}

// Fungsi untuk membuka dan menutup servo
void feedFish() {
    myservo.write(45);
    delay(1000);
    myservo.write(90);
}