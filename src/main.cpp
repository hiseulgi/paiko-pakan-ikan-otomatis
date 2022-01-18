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

    // Mengambil waktu pakan otomatis (alarm) dari firebase
    Firebase.getString(fbdo, "waktuPakan/waktu1");
    alarm1 = fbdo.to<const char *>();
    hr1 = alarm1.substring(0, 2).toInt();
    min1 = alarm1.substring(3, 5).toInt();

    Firebase.getString(fbdo, "waktuPakan/waktu2");
    alarm2 = fbdo.to<const char *>();
    hr2 = alarm2.substring(0, 2).toInt();
    min2 = alarm2.substring(3, 5).toInt();
}

void loop() {
    // Reading button state
    btnState = digitalRead(btnPin);

    // Update nilai datetime dari internet
    timeClient.update();

    String hari = weekDays[timeClient.getDay()];
    String tanggal = timeClient.getFormattedDate();
    String waktu = timeClient.getFormattedTime();
    int hrNow = waktu.substring(0, 2).toInt();
    int minNow = waktu.substring(3, 5).toInt();

    // Telegram Bot
    handleNewMessages(hari, tanggal, waktu);

    // Handle Input dari button
    if (!btnState) {
        feedFish();
        pushHistory(hari, tanggal, waktu);
        delay(500);
    }

    // Handle pemberi pakan ikan otomatis (alarm)
    handleAlarm(hrNow, minNow, hari, tanggal, waktu);

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

            Firebase.getString(fbdo, "waktuPakan/waktu1");
            String alarm1 = fbdo.to<const char *>();
            Firebase.getString(fbdo, "waktuPakan/waktu2");
            String alarm2 = fbdo.to<const char *>();

            String txt = "Paiko terakhir kali memberikan pakan ikan pada : \n\n";
            txt += "Hari          : " + day;
            txt += "\nTanggal   : " + date;
            txt += "\nWaktu      : " + time;
            txt += "\n\nWaktu pemberian pakan secara otomatis pada :\n";
            txt += "- Pukul " + alarm1 + " WIB";
            txt += "\n- Pukul " + alarm2 + " WIB";
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
            String txt = "Atur waktu penjadwalan otomatis.\n";
            txt += "Silahkan pilih waktu yang ingin diubah(1/2)\n";
            txt += "/1 - waktu pertama\n";
            txt += "/2 - waktu kedua";
            myBot.sendMessage(chat_id, txt);
            botState = 1;
            setTimeState = 0;
            return;
        }

        // settime pertama
        if (msg.text.equalsIgnoreCase("/1") && botState) {
            setTimeState = 1;
            String txt = "1. Waktu pertama\n";
            txt += "Silahkan masukkan waktu dengan format hh:mm (ex : 19:00)";
            myBot.sendMessage(chat_id, txt);
            return;
        }

        if (botState && (setTimeState == 1)) {
            String tempTime = msg.text.substring(0, 5);
            if (!validateTime(tempTime)) {
                myBot.sendMessage(chat_id, "Format yang kakak masukkan salah!");
                return;
            }

            Firebase.setString(fbdo, "waktuPakan/waktu1", tempTime);
            botState = 0;
            setTimeState = 0;

            alarm1 = tempTime;
            hr1 = tempTime.substring(0, 2).toInt();
            min1 = tempTime.substring(3, 5).toInt();
            String txt = "Waktu " + tempTime + " telah diset sebagai waktu pertama.";
            myBot.sendMessage(chat_id, txt);
            return;
        }
        // =====================

        // settime kedua
        if (msg.text.equalsIgnoreCase("/2") && botState) {
            setTimeState = 2;
            String txt = "2. Waktu kedua\n";
            txt += "Silahkan masukkan waktu dengan format hh:mm (ex : 19:00)";
            myBot.sendMessage(chat_id, txt);
            return;
        }

        if (botState && (setTimeState == 2)) {
            String tempTime = msg.text.substring(0, 5);
            if (!validateTime(tempTime)) {
                myBot.sendMessage(chat_id, "Format yang kakak masukkan salah! 😊");
                return;
            }

            Firebase.setString(fbdo, "waktuPakan/waktu2", tempTime);
            botState = 0;
            setTimeState = 0;

            alarm2 = tempTime;
            hr2 = tempTime.substring(0, 2).toInt();
            min2 = tempTime.substring(3, 5).toInt();
            String txt = "Waktu " + tempTime + " telah diset sebagai waktu kedua.";
            myBot.sendMessage(chat_id, txt);
            return;
        }
        // =====================

        // default argument
        botState = 0;
        setTimeState = 0;
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

// fungsi untuk validasi waktu
bool validateTime(String waktu) {
    // validasi panjang string
    if (waktu.length() != 5) {
        return 0;
    }

    // validasi titik dua (:)
    if (waktu.substring(2, 3) != ":") {
        return 0;
    }

    // validasi jam dan menit
    int hr = waktu.substring(0, 2).toInt();
    int min = waktu.substring(3, 5).toInt();

    if (hr >= 0 && hr < 24) {
        if (min >= 0 && min < 60) {
            return 1;
        }
    }

    return 0;
}

// Fungsi untuk handle pemberi pakan ikan otomatis jika sesuai waktu pada firebase (alarm)
void handleAlarm(int hour, int minute, String hari, String tanggal, String waktu) {
    if (alarmState) {
        // cek apakah waktu skrng sama dengan waktu pada firebase
        if (hour == hr1 && minute == min1) {
            feedFish();
            pushHistory(hari, tanggal, waktu);
            delay(500);
            alarmState = 0;
        } else if (hour == hr2 && minute == min2) {
            feedFish();
            pushHistory(hari, tanggal, waktu);
            delay(500);
            alarmState = 0;
        }
    }

    // membuat var alarmState = true dengan menunggu 1 menit dr waktu alarm
    // ------> agar alarm berikutnya dapat menyala
    if (hour == hr1 && minute == min1 + 1 ||
        hour == hr2 && minute == min2 + 1) {
        alarmState = true;
    }
}